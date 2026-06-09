#include "mesh.h"

#include <glm/gtc/constants.hpp>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstddef>
#include <cctype>

#include "texture.h"

namespace {
std::string toLower(std::string s)
{
    for (char& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

bool isWalkSurfaceName(const std::string& meshName)
{
    const std::string n = toLower(meshName);
    if (n.find("envelope") != std::string::npos) return false;
    if (n.find("vegetation") != std::string::npos) return false;
    return n.find("terrain") != std::string::npos ||
           n.find("road") != std::string::npos ||
           n.find("path") != std::string::npos ||
           n.find("pedestrian") != std::string::npos;
}

static const glm::vec3 HENRY_P1(-124.538f, 67.4345f, 122.907f);
static const glm::vec3 HENRY_P2(-65.1737f, 67.2052f, 125.271f);
static const glm::vec3 HENRY_P3(-48.005f, 60.3155f, 190.264f);
static const glm::vec3 HENRY_P4(-121.389f, 67.0113f, 186.591f);
static constexpr float HENRY_TOP_STEP_BAND = 0.26f;
static constexpr float HENRY_LEFT_STEP_BAND = 0.24f;
static constexpr float HENRY_EDGE_STEP_DROP = 4.70f;
static constexpr int HENRY_EDGE_STEPS = 10;

bool isOdeonName(const std::string& meshName)
{
    return toLower(meshName).find("odeon") != std::string::npos;
}

bool isBuildingName(const std::string& meshName)
{
    const std::string n = toLower(meshName);
    return n.find("building") != std::string::npos;
}

bool isRoadName(const std::string& meshName)
{
    const std::string n = toLower(meshName);
    return n.find("road") != std::string::npos ||
           n.find("track") != std::string::npos;
}

bool isPathName(const std::string& meshName)
{
    const std::string n = toLower(meshName);
    return n.find("pedestrian") != std::string::npos ||
           n.find("path") != std::string::npos ||
           n.find("footway") != std::string::npos;
}

bool isVegetationName(const std::string& meshName)
{
    return toLower(meshName).find("vegetation") != std::string::npos;
}

bool shouldSkipRenderableMesh(const std::string& meshName)
{
    const std::string n = toLower(meshName);
    return n.find("envelope") != std::string::npos ||
           n.find("boundingbox") != std::string::npos ||
           n.find("bounding_box") != std::string::npos ||
           n.find("bounding box") != std::string::npos ||
           n.find("bbox") != std::string::npos;
}

// Hash a building index into a deterministic facade color from a natural palette
glm::vec3 buildingWallColor(unsigned int seed)
{
    // Palette of realistic facade colors (concrete, stone, plaster)
    static const glm::vec3 palette[] = {
        {0.70f, 0.66f, 0.58f},  // warm stone
        {0.64f, 0.62f, 0.58f},  // concrete grey
        {0.73f, 0.68f, 0.58f},  // sandy beige
        {0.76f, 0.72f, 0.65f},  // limestone
        {0.60f, 0.58f, 0.54f},  // shaded concrete
        {0.68f, 0.62f, 0.52f},  // tan stone
        {0.66f, 0.64f, 0.60f},  // light grey
        {0.72f, 0.65f, 0.56f},  // sandstone
    };
    return palette[seed % 8];
}

glm::vec3 buildingRoofColor(unsigned int seed)
{
    static const glm::vec3 palette[] = {
        {0.62f, 0.23f, 0.12f},  // terracotta red
        {0.72f, 0.32f, 0.15f},  // sunlit clay
        {0.52f, 0.18f, 0.10f},  // aged red tile
        {0.68f, 0.27f, 0.11f},  // warm roof tile
    };
    return palette[seed % 4];
}

// Extract building index from names like "map.osm_buildings.017-0"
unsigned int buildingIndex(const std::string& name)
{
    auto pos = name.find("buildings");
    if (pos == std::string::npos) return 0;
    pos += 9; // skip "buildings"
    if (pos < name.size() && name[pos] == '.') pos++;
    unsigned int idx = 0;
    while (pos < name.size() && name[pos] >= '0' && name[pos] <= '9') {
        idx = idx * 10 + (name[pos] - '0');
        pos++;
    }
    return idx;
}

bool isBuildingRoof(const std::string& meshName)
{
    const std::string n = toLower(meshName);
    return n.find("roof") != std::string::npos ||
           (n.size() >= 2 && n.substr(n.size() - 2) == "-1");
}

glm::vec3 assignColor(const std::string& meshName)
{
    const std::string n = toLower(meshName);

    if (isOdeonName(n)) {
        if (n.find("structure") != std::string::npos) {
            return glm::vec3(0.55f, 0.50f, 0.44f);
        }
        return glm::vec3(0.68f, 0.64f, 0.57f);
    }
    if (n.find("map.osm_buildings.022") != std::string::npos) {
        if (isBuildingRoof(n)) {
            return glm::vec3(0.64f, 0.24f, 0.11f);
        }
        return glm::vec3(0.78f, 0.73f, 0.63f);
    }
    if (n.find("building") != std::string::npos) {
        unsigned int idx = buildingIndex(n);
        if (isBuildingRoof(n)) {
            return buildingRoofColor(idx);
        }
        return buildingWallColor(idx);
    }
    if (n.find("vegetation") != std::string::npos) {
        return glm::vec3(0.30f, 0.50f, 0.25f);  // natural green
    }
    if (isRoadName(n)) {
        return glm::vec3(0.30f, 0.31f, 0.32f);  // asphalt
    }
    if (isPathName(n)) {
        return glm::vec3(0.58f, 0.56f, 0.50f);  // light concrete
    }
    if (n.find("envelope") != std::string::npos) {
        return glm::vec3(0.45f, 0.42f, 0.35f);  // earthy brown (visible at horizon)
    }

    return glm::vec3(1.0f);  // default white (let texture show through)
}

bool isGenericMaterialColor(const aiColor3D& color)
{
    const float maxChannel = std::max({color.r, color.g, color.b});
    const float minChannel = std::min({color.r, color.g, color.b});
    const bool nearlyGrey = (maxChannel - minChannel) < 0.04f;
    return nearlyGrey && maxChannel > 0.72f;
}

bool shouldKeepSemanticColor(const std::string& meshName)
{
    return isBuildingName(meshName) ||
           isRoadName(meshName) ||
           isPathName(meshName) ||
           isOdeonName(meshName) ||
           toLower(meshName).find("vegetation") != std::string::npos;
}

float averageNormalY(const std::vector<Vertex>& vertices)
{
    if (vertices.empty()) {
        return 0.0f;
    }
    float sum = 0.0f;
    for (const auto& v : vertices) {
        sum += v.normal.y;
    }
    return sum / static_cast<float>(vertices.size());
}

int materialModeForMesh(const std::string& meshName, const std::vector<Vertex>& vertices)
{
    if (isVegetationName(meshName)) {
        return 5;
    }
    if (isBuildingName(meshName)) {
        if (isBuildingRoof(meshName) || averageNormalY(vertices) > 0.55f) {
            return 2;
        }
        return 1;
    }
    if (isRoadName(meshName) || isPathName(meshName)) {
        return 3;
    }
    return 0;
}

bool isRectorateMeshName(const std::string& meshName)
{
    return toLower(meshName).find("map.osm_buildings.022") != std::string::npos;
}

bool isKocDetailMeshName(const std::string& meshName)
{
    return toLower(meshName).find("koc_") != std::string::npos;
}

struct RectorateLayout {
    float xMin = 0.0f;
    float xMax = 0.0f;
    float zMin = 0.0f;
    float zMax = 0.0f;
    float centerMinZ = 0.0f;
    float centerMaxZ = 0.0f;
    float gateMinZ = 0.0f;
    float gateMaxZ = 0.0f;
    float baseY = 0.0f;
    float sideTopY = 0.0f;
    float archTopY = 0.0f;
    float centerTopY = 0.0f;
};

bool hasRectorateLayout = false;
RectorateLayout rectorateLayout{};

void computeBounds(const std::vector<Vertex>& vertices, glm::vec3& outMin, glm::vec3& outMax)
{
    outMin = glm::vec3(std::numeric_limits<float>::max());
    outMax = glm::vec3(std::numeric_limits<float>::lowest());
    for (const auto& v : vertices) {
        outMin = glm::min(outMin, v.pos);
        outMax = glm::max(outMax, v.pos);
    }
}

RectorateLayout makeRectorateLayout(const glm::vec3& meshMin, const glm::vec3& meshMax)
{
    RectorateLayout layout{};
    layout.xMin = meshMin.x;
    layout.xMax = meshMax.x;
    layout.zMin = meshMin.z;
    layout.zMax = meshMax.z;

    const float spanZ = meshMax.z - meshMin.z;
    const float centerZ = (meshMin.z + meshMax.z) * 0.5f;
    const float gateHalfZ = std::clamp(spanZ * 0.115f, 5.0f, 7.5f);
    const float centerHalfZ = std::clamp(spanZ * 0.17f, gateHalfZ + 2.75f, spanZ * 0.28f);
    layout.gateMinZ = centerZ - gateHalfZ;
    layout.gateMaxZ = centerZ + gateHalfZ;
    layout.centerMinZ = centerZ - centerHalfZ;
    layout.centerMaxZ = centerZ + centerHalfZ;

    const float height = meshMax.y - meshMin.y;
    if (height > 5.0f) {
        layout.baseY = meshMin.y;
        layout.sideTopY = meshMin.y + height * 0.78f;
        layout.archTopY = meshMin.y + height * 0.58f;
        layout.centerTopY = meshMin.y + height * 1.27f;
    } else {
        layout.baseY = meshMax.y - 18.9f;
        layout.sideTopY = meshMax.y - 4.2f;
        layout.archTopY = layout.baseY + 11.0f;
        layout.centerTopY = meshMax.y + 5.1f;
    }
    return layout;
}

void addQuad(std::vector<Vertex>& vertices,
             std::vector<unsigned int>& indices,
             const glm::vec3& a,
             const glm::vec3& b,
             const glm::vec3& c,
             const glm::vec3& d,
             const glm::vec3& normal)
{
    const unsigned int start = static_cast<unsigned int>(vertices.size());
    vertices.push_back({a, normal, glm::vec2(0.0f, 0.0f)});
    vertices.push_back({b, normal, glm::vec2(1.0f, 0.0f)});
    vertices.push_back({c, normal, glm::vec2(1.0f, 1.0f)});
    vertices.push_back({d, normal, glm::vec2(0.0f, 1.0f)});
    indices.insert(indices.end(), {
        start, start + 1, start + 2,
        start, start + 2, start + 3
    });
}

void addBox(std::vector<Vertex>& vertices,
            std::vector<unsigned int>& indices,
            float xMin,
            float xMax,
            float yMin,
            float yMax,
            float zMin,
            float zMax,
            bool includeTop,
            bool includeBottom)
{
    addQuad(vertices, indices,
            glm::vec3(xMax, yMin, zMin), glm::vec3(xMax, yMin, zMax),
            glm::vec3(xMax, yMax, zMax), glm::vec3(xMax, yMax, zMin),
            glm::vec3(1.0f, 0.0f, 0.0f));
    addQuad(vertices, indices,
            glm::vec3(xMin, yMin, zMax), glm::vec3(xMin, yMin, zMin),
            glm::vec3(xMin, yMax, zMin), glm::vec3(xMin, yMax, zMax),
            glm::vec3(-1.0f, 0.0f, 0.0f));
    addQuad(vertices, indices,
            glm::vec3(xMin, yMin, zMax), glm::vec3(xMax, yMin, zMax),
            glm::vec3(xMax, yMax, zMax), glm::vec3(xMin, yMax, zMax),
            glm::vec3(0.0f, 0.0f, 1.0f));
    addQuad(vertices, indices,
            glm::vec3(xMax, yMin, zMin), glm::vec3(xMin, yMin, zMin),
            glm::vec3(xMin, yMax, zMin), glm::vec3(xMax, yMax, zMin),
            glm::vec3(0.0f, 0.0f, -1.0f));

    if (includeTop) {
        addQuad(vertices, indices,
                glm::vec3(xMin, yMax, zMin), glm::vec3(xMax, yMax, zMin),
                glm::vec3(xMax, yMax, zMax), glm::vec3(xMin, yMax, zMax),
                glm::vec3(0.0f, 1.0f, 0.0f));
    }
    if (includeBottom) {
        addQuad(vertices, indices,
                glm::vec3(xMin, yMin, zMax), glm::vec3(xMax, yMin, zMax),
                glm::vec3(xMax, yMin, zMin), glm::vec3(xMin, yMin, zMin),
                glm::vec3(0.0f, -1.0f, 0.0f));
    }
}

glm::vec3 henryFordPoint(const glm::vec3& p1,
                         const glm::vec3& p2,
                         const glm::vec3& p3,
                         const glm::vec3& p4,
                         float s,
                         float t,
                         float y)
{
    const glm::vec3 top = glm::mix(p1, p2, s);
    const glm::vec3 bottom = glm::mix(p4, p3, s);
    const glm::vec3 xz = glm::mix(top, bottom, t);
    return glm::vec3(xz.x, y, xz.z);
}

void addAutoQuad(std::vector<Vertex>& vertices,
                 std::vector<unsigned int>& indices,
                 const glm::vec3& a,
                 const glm::vec3& b,
                 const glm::vec3& c,
                 const glm::vec3& d)
{
    glm::vec3 normal = glm::cross(b - a, c - a);
    normal = (glm::length(normal) > 1e-6f) ? glm::normalize(normal) : glm::vec3(0.0f, 1.0f, 0.0f);
    addQuad(vertices, indices, a, b, c, d, normal);
}

void addHenryFordHorizontal(std::vector<Vertex>& vertices,
                            std::vector<unsigned int>& indices,
                            const glm::vec3& p1,
                            const glm::vec3& p2,
                            const glm::vec3& p3,
                            const glm::vec3& p4,
                            float sMin,
                            float sMax,
                            float tMin,
                            float tMax,
                            float y)
{
    addQuad(vertices, indices,
            henryFordPoint(p1, p2, p3, p4, sMin, tMin, y),
            henryFordPoint(p1, p2, p3, p4, sMax, tMin, y),
            henryFordPoint(p1, p2, p3, p4, sMax, tMax, y),
            henryFordPoint(p1, p2, p3, p4, sMin, tMax, y),
            glm::vec3(0.0f, 1.0f, 0.0f));
}

void addHenryFordRiserT(std::vector<Vertex>& vertices,
                        std::vector<unsigned int>& indices,
                        const glm::vec3& p1,
                        const glm::vec3& p2,
                        const glm::vec3& p3,
                        const glm::vec3& p4,
                        float sMin,
                        float sMax,
                        float t,
                        float yMin,
                        float yMax)
{
    addAutoQuad(vertices, indices,
                henryFordPoint(p1, p2, p3, p4, sMax, t, yMin),
                henryFordPoint(p1, p2, p3, p4, sMin, t, yMin),
                henryFordPoint(p1, p2, p3, p4, sMin, t, yMax),
                henryFordPoint(p1, p2, p3, p4, sMax, t, yMax));
}

void addHenryFordRiserS(std::vector<Vertex>& vertices,
                        std::vector<unsigned int>& indices,
                        const glm::vec3& p1,
                        const glm::vec3& p2,
                        const glm::vec3& p3,
                        const glm::vec3& p4,
                        float s,
                        float tMin,
                        float tMax,
                        float yMin,
                        float yMax)
{
    addAutoQuad(vertices, indices,
                henryFordPoint(p1, p2, p3, p4, s, tMin, yMin),
                henryFordPoint(p1, p2, p3, p4, s, tMax, yMin),
                henryFordPoint(p1, p2, p3, p4, s, tMax, yMax),
                henryFordPoint(p1, p2, p3, p4, s, tMin, yMax));
}

bool pointInTriangleXZ(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b, const glm::vec2& c)
{
    const auto sign = [](const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3) {
        return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
    };
    const float d1 = sign(p, a, b);
    const float d2 = sign(p, b, c);
    const float d3 = sign(p, c, a);
    const bool hasNeg = (d1 < 0.0f) || (d2 < 0.0f) || (d3 < 0.0f);
    const bool hasPos = (d1 > 0.0f) || (d2 > 0.0f) || (d3 > 0.0f);
    return !(hasNeg && hasPos);
}

bool pointInQuadXZ(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& d)
{
    const glm::vec2 q(p.x, p.z);
    return pointInTriangleXZ(q, glm::vec2(a.x, a.z), glm::vec2(b.x, b.z), glm::vec2(c.x, c.z)) ||
           pointInTriangleXZ(q, glm::vec2(a.x, a.z), glm::vec2(c.x, c.z), glm::vec2(d.x, d.z));
}

void applyHenryFordTerrainFlattening(const std::string& meshName, std::vector<Vertex>& vertices)
{
    if (!(isWalkSurfaceName(meshName) || isVegetationName(meshName))) {
        return;
    }

    const float finalY = 67.45f - HENRY_EDGE_STEP_DROP;
    const float sMin = HENRY_LEFT_STEP_BAND - 0.06f;
    const float tMin = HENRY_TOP_STEP_BAND - 0.06f;
    const float sMax = 1.04f;
    const float tMax = 1.04f;
    const glm::vec3 q1 = henryFordPoint(HENRY_P1, HENRY_P2, HENRY_P3, HENRY_P4, sMin, tMin, finalY);
    const glm::vec3 q2 = henryFordPoint(HENRY_P1, HENRY_P2, HENRY_P3, HENRY_P4, sMax, tMin, finalY);
    const glm::vec3 q3 = henryFordPoint(HENRY_P1, HENRY_P2, HENRY_P3, HENRY_P4, sMax, tMax, finalY);
    const glm::vec3 q4 = henryFordPoint(HENRY_P1, HENRY_P2, HENRY_P3, HENRY_P4, sMin, tMax, finalY);

    for (auto& v : vertices) {
        if (pointInQuadXZ(v.pos, q1, q2, q3, q4)) {
            v.pos.y = finalY;
            v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
    }
}

void addDisc(std::vector<Vertex>& vertices,
             std::vector<unsigned int>& indices,
             const glm::vec3& center,
             float radius,
             int segments,
             const glm::vec3& normal)
{
    const unsigned int start = static_cast<unsigned int>(vertices.size());
    vertices.push_back({center, normal, glm::vec2(0.5f, 0.5f)});
    for (int i = 0; i <= segments; ++i) {
        const float a = (2.0f * glm::pi<float>() * static_cast<float>(i)) / static_cast<float>(segments);
        const float x = center.x + std::cos(a) * radius;
        const float z = center.z + std::sin(a) * radius;
        const glm::vec2 uv(0.5f + std::cos(a) * 0.5f, 0.5f + std::sin(a) * 0.5f);
        vertices.push_back({glm::vec3(x, center.y, z), normal, uv});
    }
    for (int i = 1; i <= segments; ++i) {
        indices.insert(indices.end(), {start, start + i, start + i + 1});
    }
}

void addRingWall(std::vector<Vertex>& vertices,
                 std::vector<unsigned int>& indices,
                 const glm::vec3& center,
                 float radius,
                 float yMin,
                 float yMax,
                 int segments)
{
    for (int i = 0; i < segments; ++i) {
        const float a0 = (2.0f * glm::pi<float>() * static_cast<float>(i)) / static_cast<float>(segments);
        const float a1 = (2.0f * glm::pi<float>() * static_cast<float>(i + 1)) / static_cast<float>(segments);
        const glm::vec3 p0(center.x + std::cos(a0) * radius, yMin, center.z + std::sin(a0) * radius);
        const glm::vec3 p1(center.x + std::cos(a1) * radius, yMin, center.z + std::sin(a1) * radius);
        const glm::vec3 p2(center.x + std::cos(a1) * radius, yMax, center.z + std::sin(a1) * radius);
        const glm::vec3 p3(center.x + std::cos(a0) * radius, yMax, center.z + std::sin(a0) * radius);
        const glm::vec3 normal = glm::normalize(glm::vec3(std::cos((a0 + a1) * 0.5f), 0.0f, std::sin((a0 + a1) * 0.5f)));
        addQuad(vertices, indices, p0, p1, p2, p3, normal);
    }
}

Mesh uploadColoredMesh(const std::vector<Vertex>& vertices,
                       const std::vector<unsigned int>& indices,
                       const glm::vec3& color,
                       int materialMode)
{
    Mesh mesh{};
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glBindVertexArray(0);

    mesh.indexCount = static_cast<GLsizei>(indices.size());
    mesh.baseColor = color;
    glm::vec3 meshMin;
    glm::vec3 meshMax;
    computeBounds(vertices, meshMin, meshMax);
    mesh.bboxMin = meshMin;
    mesh.bboxMax = meshMax;
    mesh.boundsCenter = (meshMin + meshMax) * 0.5f;
    mesh.boundsRadius = glm::length(meshMax - mesh.boundsCenter);
    mesh.materialMode = materialMode;
    return mesh;
}

void appendCustomMesh(Model& model,
                      glm::vec3& bboxMin,
                      glm::vec3& bboxMax,
                      const std::vector<Vertex>& vertices,
                      const std::vector<unsigned int>& indices,
                      const glm::vec3& color,
                      int materialMode,
                      bool walkable,
                      bool obstacle)
{
    if (vertices.empty() || indices.empty()) {
        return;
    }

    glm::vec3 meshMin;
    glm::vec3 meshMax;
    computeBounds(vertices, meshMin, meshMax);
    bboxMin = glm::min(bboxMin, meshMin);
    bboxMax = glm::max(bboxMax, meshMax);

    if (walkable) {
        for (std::size_t i = 0; i + 2 < indices.size(); i += 3) {
            model.walkSurface.push_back({
                vertices[indices[i]].pos,
                vertices[indices[i + 1]].pos,
                vertices[indices[i + 2]].pos
            });
        }
    }

    if (obstacle) {
        model.obstacles.push_back({
            glm::vec2(meshMin.x, meshMin.z),
            glm::vec2(meshMax.x, meshMax.z),
            meshMin.y,
            meshMax.y
        });
    }

    model.meshes.push_back(uploadColoredMesh(vertices, indices, color, materialMode));
}

bool isCampusScenePath(const std::string& path)
{
    const std::string p = toLower(path);
    return p.find("campus.") != std::string::npos ||
           p.find("3_7_2026.") != std::string::npos;
}

void appendHenryFordSiteDetails(Model& model, glm::vec3& bboxMin, glm::vec3& bboxMax)
{
    std::vector<Vertex> treadVertices;
    std::vector<unsigned int> treadIndices;
    std::vector<Vertex> riserVertices;
    std::vector<unsigned int> riserIndices;

    const float highY = 67.45f;

    for (int i = 0; i < HENRY_EDGE_STEPS; ++i) {
        const float t0 = HENRY_TOP_STEP_BAND * static_cast<float>(i) / static_cast<float>(HENRY_EDGE_STEPS);
        const float t1 = HENRY_TOP_STEP_BAND * static_cast<float>(i + 1) / static_cast<float>(HENRY_EDGE_STEPS);
        const float y = highY - HENRY_EDGE_STEP_DROP * static_cast<float>(i) / static_cast<float>(HENRY_EDGE_STEPS);
        const float nextY = highY - HENRY_EDGE_STEP_DROP * static_cast<float>(i + 1) / static_cast<float>(HENRY_EDGE_STEPS);
        addHenryFordHorizontal(treadVertices, treadIndices, HENRY_P1, HENRY_P2, HENRY_P3, HENRY_P4,
                               0.0f, 1.0f, t0, t1, y);
        addHenryFordRiserT(riserVertices, riserIndices, HENRY_P1, HENRY_P2, HENRY_P3, HENRY_P4,
                           0.0f, 1.0f, t1, nextY, y);
    }

    for (int i = 0; i < HENRY_EDGE_STEPS; ++i) {
        const float s0 = HENRY_LEFT_STEP_BAND * static_cast<float>(i) / static_cast<float>(HENRY_EDGE_STEPS);
        const float s1 = HENRY_LEFT_STEP_BAND * static_cast<float>(i + 1) / static_cast<float>(HENRY_EDGE_STEPS);
        const float y = highY - HENRY_EDGE_STEP_DROP * static_cast<float>(i) / static_cast<float>(HENRY_EDGE_STEPS);
        const float nextY = highY - HENRY_EDGE_STEP_DROP * static_cast<float>(i + 1) / static_cast<float>(HENRY_EDGE_STEPS);
        addHenryFordHorizontal(treadVertices, treadIndices, HENRY_P1, HENRY_P2, HENRY_P3, HENRY_P4,
                               s0, s1, HENRY_TOP_STEP_BAND, 1.0f, y);
        addHenryFordRiserS(riserVertices, riserIndices, HENRY_P1, HENRY_P2, HENRY_P3, HENRY_P4,
                           s1, HENRY_TOP_STEP_BAND, 1.0f, nextY, y);
    }

    appendCustomMesh(model, bboxMin, bboxMax, treadVertices, treadIndices,
                     glm::vec3(0.78f, 0.73f, 0.66f), 12, true, false);
    appendCustomMesh(model, bboxMin, bboxMax, riserVertices, riserIndices,
                     glm::vec3(0.46f, 0.42f, 0.37f), 12, false, false);
}

void appendRectorateFountainDetails(Model& model, glm::vec3& bboxMin, glm::vec3& bboxMax)
{
    if (!hasRectorateLayout) {
        return;
    }

    const float gateCenterZ = (rectorateLayout.gateMinZ + rectorateLayout.gateMaxZ) * 0.5f;
    const float xCenter = rectorateLayout.xMin - 22.5f;
    const float basinTop = rectorateLayout.baseY + 0.55f;
    const float basinBottom = rectorateLayout.baseY - 0.10f;
    const float outerRadius = 4.70f;
    const float innerRadius = 3.64f;
    const int segments = 32;
    const glm::vec3 center(xCenter, basinTop, gateCenterZ);

    std::vector<Vertex> stoneVertices;
    std::vector<unsigned int> stoneIndices;
    std::vector<Vertex> waterVertices;
    std::vector<unsigned int> waterIndices;

    addDisc(stoneVertices, stoneIndices, center, outerRadius, segments, glm::vec3(0.0f, 1.0f, 0.0f));
    addRingWall(stoneVertices, stoneIndices, center, outerRadius, basinBottom, basinTop, segments);
    addRingWall(stoneVertices, stoneIndices, center, innerRadius, basinBottom, basinTop - 0.08f, segments);

    const glm::vec3 waterCenter(center.x, basinTop - 0.18f, center.z);
    addDisc(waterVertices, waterIndices, waterCenter, innerRadius - 0.05f, segments, glm::vec3(0.0f, 1.0f, 0.0f));

    appendCustomMesh(model, bboxMin, bboxMax, stoneVertices, stoneIndices,
                     glm::vec3(0.72f, 0.70f, 0.66f), 6, false, false);
    appendCustomMesh(model, bboxMin, bboxMax, waterVertices, waterIndices,
                     glm::vec3(0.18f, 0.40f, 0.58f), 11, false, false);
}

void appendFenceSegment(std::vector<Vertex>& vertices,
                        std::vector<unsigned int>& indices,
                        const glm::vec3& a,
                        const glm::vec3& b,
                        float fenceHeight)
{
    const glm::vec3 delta = b - a;
    const float length = glm::length(glm::vec2(delta.x, delta.z));
    if (length < 1e-4f) {
        return;
    }
    const glm::vec3 dir = glm::normalize(glm::vec3(delta.x, 0.0f, delta.z));
    const glm::vec3 right(-dir.z, 0.0f, dir.x);
    const float postSpacing = 6.0f;
    const int posts = std::max(1, static_cast<int>(std::floor(length / postSpacing)));
    for (int i = 0; i <= posts; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(posts);
        const glm::vec3 p = glm::mix(a, b, t);
        addBox(vertices, indices, p.x - 0.08f, p.x + 0.08f, p.y, p.y + fenceHeight,
               p.z - 0.08f, p.z + 0.08f, true, false);
        if (i < posts) {
            const glm::vec3 q = glm::mix(a, b, static_cast<float>(i + 1) / static_cast<float>(posts));
            const glm::vec3 half = right * 0.05f;
            addQuad(vertices, indices,
                    glm::vec3(p.x, p.y + 0.30f, p.z) - half,
                    glm::vec3(q.x, q.y + 0.30f, q.z) - half,
                    glm::vec3(q.x, q.y + fenceHeight - 0.15f, q.z) + half,
                    glm::vec3(p.x, p.y + fenceHeight - 0.15f, p.z) + half,
                    right);
        }
    }
}

void appendCampusFenceAndForest(Model& model, glm::vec3& bboxMin, glm::vec3& bboxMax)
{
    if (model.roadPoints.size() < 32) {
        return;
    }

    const glm::vec3 center = (bboxMin + bboxMax) * 0.5f;
    std::vector<float> radii;
    radii.reserve(model.roadPoints.size());
    float maxRadius = 0.0f;
    for (const auto& p : model.roadPoints) {
        const float r = glm::length(glm::vec2(p.x - center.x, p.z - center.z));
        radii.push_back(r);
        maxRadius = std::max(maxRadius, r);
    }
    std::sort(radii.begin(), radii.end());
    const float medianRadius = radii[radii.size() / 2];
    const float innerRingMin = medianRadius * 0.95f;
    const float innerRingMax = maxRadius * 0.78f;

    struct RadialPoint {
        bool valid = false;
        glm::vec3 point = glm::vec3(0.0f);
        float radius = 0.0f;
    };
    constexpr int BIN_COUNT = 72;
    std::array<RadialPoint, BIN_COUNT> bins{};
    for (const auto& p : model.roadPoints) {
        const glm::vec2 d(p.x - center.x, p.z - center.z);
        const float radius = glm::length(d);
        if (radius < innerRingMin || radius > innerRingMax) {
            continue;
        }
        float angle = std::atan2(d.y, d.x);
        if (angle < 0.0f) {
            angle += 2.0f * glm::pi<float>();
        }
        int bin = static_cast<int>(std::floor((angle / (2.0f * glm::pi<float>())) * BIN_COUNT)) % BIN_COUNT;
        if (!bins[bin].valid || radius > bins[bin].radius) {
            bins[bin].valid = true;
            bins[bin].point = p;
            bins[bin].radius = radius;
        }
    }

    std::vector<glm::vec3> fenceLoop;
    fenceLoop.reserve(BIN_COUNT);
    for (int i = 0; i < BIN_COUNT; ++i) {
        if (!bins[i].valid) {
            continue;
        }
        glm::vec3 p = bins[i].point;
        glm::vec3 outward = glm::normalize(glm::vec3(p.x - center.x, 0.0f, p.z - center.z));
        if (glm::length(outward) < 1e-4f) {
            outward = glm::vec3(1.0f, 0.0f, 0.0f);
        }
        p += outward * 2.5f;
        p.y += 0.05f;
        fenceLoop.push_back(p);
    }

    if (fenceLoop.size() < 8) {
        return;
    }

    model.perimeterPoints = fenceLoop;

    std::vector<Vertex> fenceVertices;
    std::vector<unsigned int> fenceIndices;
    for (std::size_t i = 0; i < fenceLoop.size(); ++i) {
        appendFenceSegment(fenceVertices, fenceIndices,
                           fenceLoop[i], fenceLoop[(i + 1) % fenceLoop.size()], 2.0f);
    }
    appendCustomMesh(model, bboxMin, bboxMax, fenceVertices, fenceIndices,
                     glm::vec3(0.20f, 0.22f, 0.20f), 13, false, false);
}

void buildRectorateWallGeometry(const RectorateLayout& layout,
                                std::vector<Vertex>& vertices,
                                std::vector<unsigned int>& indices)
{
    vertices.clear();
    indices.clear();

    addBox(vertices, indices,
           layout.xMin, layout.xMax, layout.baseY, layout.sideTopY,
           layout.zMin, layout.centerMinZ, false, false);
    addBox(vertices, indices,
           layout.xMin, layout.xMax, layout.baseY, layout.sideTopY,
           layout.centerMaxZ, layout.zMax, false, false);

    addBox(vertices, indices,
           layout.xMin, layout.xMax, layout.baseY, layout.centerTopY,
           layout.centerMinZ, layout.gateMinZ, false, false);
    addBox(vertices, indices,
           layout.xMin, layout.xMax, layout.baseY, layout.centerTopY,
           layout.gateMaxZ, layout.centerMaxZ, false, false);
    addBox(vertices, indices,
           layout.xMin, layout.xMax, layout.archTopY, layout.centerTopY,
           layout.gateMinZ, layout.gateMaxZ, false, true);
}

void buildRectorateRoofGeometry(const RectorateLayout& layout,
                                std::vector<Vertex>& vertices,
                                std::vector<unsigned int>& indices)
{
    vertices.clear();
    indices.clear();

    const float xOverhang = std::min((layout.xMax - layout.xMin) * 0.08f, 1.2f);
    const float zOverhang = 0.9f;
    const float sideRoofThickness = 0.65f;
    const float centerRoofThickness = 0.75f;

    addBox(vertices, indices,
           layout.xMin - xOverhang, layout.xMax + xOverhang,
           layout.sideTopY, layout.sideTopY + sideRoofThickness,
           layout.zMin - zOverhang, layout.centerMinZ + 0.25f,
           true, false);
    addBox(vertices, indices,
           layout.xMin - xOverhang, layout.xMax + xOverhang,
           layout.sideTopY, layout.sideTopY + sideRoofThickness,
           layout.centerMaxZ - 0.25f, layout.zMax + zOverhang,
           true, false);
    addBox(vertices, indices,
           layout.xMin - xOverhang, layout.xMax + xOverhang,
           layout.centerTopY, layout.centerTopY + centerRoofThickness,
           layout.centerMinZ - zOverhang, layout.centerMaxZ + zOverhang,
           true, false);
}

bool isInsideRectorateGateOpening(const glm::vec3& p)
{
    if (!hasRectorateLayout) {
        return false;
    }

    return p.x >= rectorateLayout.xMin - 0.35f &&
           p.x <= rectorateLayout.xMax + 0.35f &&
           p.z >= rectorateLayout.gateMinZ - 0.35f &&
           p.z <= rectorateLayout.gateMaxZ + 0.35f &&
           p.y >= rectorateLayout.baseY - 0.35f &&
           p.y <= rectorateLayout.archTopY + 0.45f;
}

bool shouldCullRectorateGateDetail(const std::string& meshName,
                                   const Vertex& a,
                                   const Vertex& b,
                                   const Vertex& c)
{
    if (!isKocDetailMeshName(meshName)) {
        return false;
    }

    const glm::vec3 centroid = (a.pos + b.pos + c.pos) / 3.0f;
    return isInsideRectorateGateOpening(centroid);
}

void appendObstacle(Model& model,
                    float xMin,
                    float xMax,
                    float zMin,
                    float zMax,
                    float yMin,
                    float yMax)
{
    if (xMax <= xMin || zMax <= zMin || yMax <= yMin) {
        return;
    }
    model.obstacles.push_back({
        glm::vec2(xMin, zMin),
        glm::vec2(xMax, zMax),
        yMin,
        yMax
    });
}

void appendRectorateObstacles(Model& model, const RectorateLayout& layout)
{
    appendObstacle(model, layout.xMin, layout.xMax, layout.zMin, layout.centerMinZ,
                   layout.baseY, layout.sideTopY);
    appendObstacle(model, layout.xMin, layout.xMax, layout.centerMaxZ, layout.zMax,
                   layout.baseY, layout.sideTopY);
    appendObstacle(model, layout.xMin, layout.xMax, layout.centerMinZ, layout.gateMinZ,
                   layout.baseY, layout.centerTopY);
    appendObstacle(model, layout.xMin, layout.xMax, layout.gateMaxZ, layout.centerMaxZ,
                   layout.baseY, layout.centerTopY);
    appendObstacle(model, layout.xMin, layout.xMax, layout.gateMinZ, layout.gateMaxZ,
                   layout.archTopY, layout.centerTopY);
}

Mesh buildMesh(const aiScene* scene,
               const aiMesh* ai_mesh,
               const std::filesystem::path& modelDir,
               const aiMatrix4x4& worldTransform,
               Model& model,
               std::unordered_map<std::string, GLuint>& loadedTextures,
               glm::vec3& bboxMin,
               glm::vec3& bboxMax)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(ai_mesh->mNumVertices);
    indices.reserve(ai_mesh->mNumFaces * 3);

    const std::string meshName = ai_mesh->mName.C_Str();
    aiMatrix3x3 normalMatrix(worldTransform);
    normalMatrix.Inverse().Transpose();

    for (unsigned int i = 0; i < ai_mesh->mNumVertices; ++i) {
        Vertex v{};
        const aiVector3D transformedPos = worldTransform * ai_mesh->mVertices[i];
        v.pos = glm::vec3(transformedPos.x, transformedPos.y, transformedPos.z);
        if (ai_mesh->HasNormals()) {
            aiVector3D transformedNormal = normalMatrix * ai_mesh->mNormals[i];
            const glm::vec3 n(transformedNormal.x, transformedNormal.y, transformedNormal.z);
            v.normal = (glm::length(n) > 1e-6f) ? glm::normalize(n) : glm::vec3(0.0f, 1.0f, 0.0f);
        } else {
            v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
        if (ai_mesh->HasTextureCoords(0)) {
            v.uv = glm::vec2(ai_mesh->mTextureCoords[0][i].x, ai_mesh->mTextureCoords[0][i].y);
        } else {
            v.uv = glm::vec2(0.0f);
        }

        vertices.push_back(v);
    }

    applyHenryFordTerrainFlattening(meshName, vertices);

    glm::vec3 meshMin(std::numeric_limits<float>::max());
    glm::vec3 meshMax(std::numeric_limits<float>::lowest());
    computeBounds(vertices, meshMin, meshMax);

    for (unsigned int f = 0; f < ai_mesh->mNumFaces; ++f) {
        const aiFace& face = ai_mesh->mFaces[f];
        if (face.mNumIndices == 3 &&
            shouldCullRectorateGateDetail(meshName,
                                          vertices[face.mIndices[0]],
                                          vertices[face.mIndices[1]],
                                          vertices[face.mIndices[2]])) {
            continue;
        }
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            indices.push_back(face.mIndices[j]);
        }
    }

    if (isRectorateMeshName(meshName)) {
        RectorateLayout layout = hasRectorateLayout ? rectorateLayout : makeRectorateLayout(meshMin, meshMax);
        if (isBuildingRoof(meshName)) {
            buildRectorateRoofGeometry(layout, vertices, indices);
        } else {
            layout = makeRectorateLayout(meshMin, meshMax);
            rectorateLayout = layout;
            hasRectorateLayout = true;
            buildRectorateWallGeometry(layout, vertices, indices);
        }
        computeBounds(vertices, meshMin, meshMax);
    }

    for (const auto& v : vertices) {
        bboxMin = glm::min(bboxMin, v.pos);
        bboxMax = glm::max(bboxMax, v.pos);
    }

    if (isWalkSurfaceName(meshName)) {
        for (unsigned int f = 0; f < ai_mesh->mNumFaces; ++f) {
            const aiFace& face = ai_mesh->mFaces[f];
            if (face.mNumIndices != 3) continue;
            const Vertex& v0 = vertices[face.mIndices[0]];
            const Vertex& v1 = vertices[face.mIndices[1]];
            const Vertex& v2 = vertices[face.mIndices[2]];
            model.walkSurface.push_back({v0.pos, v1.pos, v2.pos});
        }
    }

    if (isRoadName(meshName)) {
        for (const auto& v : vertices) {
            model.roadPoints.push_back(v.pos);
        }
    }

    if (isRectorateMeshName(meshName) && !isBuildingRoof(meshName) && hasRectorateLayout) {
        appendRectorateObstacles(model, rectorateLayout);
    } else if (isBuildingName(meshName)) {
        const float height = meshMax.y - meshMin.y;
        const float spanX = meshMax.x - meshMin.x;
        const float spanZ = meshMax.z - meshMin.z;
        if (height > 1.0f && spanX > 0.5f && spanZ > 0.5f) {
            model.obstacles.push_back({
                glm::vec2(meshMin.x, meshMin.z),
                glm::vec2(meshMax.x, meshMax.z),
                meshMin.y,
                meshMax.y
            });
        }
    }

    Mesh mesh{};
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glBindVertexArray(0);

    mesh.indexCount = static_cast<GLsizei>(indices.size());
    mesh.bboxMin = meshMin;
    mesh.bboxMax = meshMax;
    mesh.boundsCenter = (meshMin + meshMax) * 0.5f;
    mesh.boundsRadius = glm::length(meshMax - mesh.boundsCenter);

    // Assign a natural color based on mesh type (buildings, roads, etc.)
    mesh.baseColor = assignColor(meshName);
    mesh.materialMode = materialModeForMesh(meshName, vertices);
    if (isRectorateMeshName(meshName) && !isBuildingRoof(meshName)) {
        mesh.materialMode = 4;
    }

    if (ai_mesh->mMaterialIndex < scene->mNumMaterials) {
        aiMaterial* mat = scene->mMaterials[ai_mesh->mMaterialIndex];

        // Keep semantic colors for OSM meshes when Blender exported a generic grey material.
        aiColor3D color(1.0f, 1.0f, 1.0f);
        if (mat->Get(AI_MATKEY_BASE_COLOR, color) == AI_SUCCESS ||
            mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
            const bool isDefault = (std::abs(color.r - 1.0f) < 0.01f &&
                                    std::abs(color.g - 1.0f) < 0.01f &&
                                    std::abs(color.b - 1.0f) < 0.01f);
            const bool keepSemanticColor =
                shouldKeepSemanticColor(meshName) &&
                (isGenericMaterialColor(color) || (isBuildingName(meshName) && isBuildingRoof(meshName)));
            if (!isDefault && !keepSemanticColor) {
                mesh.baseColor = glm::vec3(color.r, color.g, color.b);
            }
        }

        aiString texPath;
        if (mat->GetTexture(aiTextureType_BASE_COLOR, 0, &texPath) != AI_SUCCESS) {
            mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath);
        }

        if (texPath.length > 0) {
            const std::string key = texPath.C_Str();
            const auto found = loadedTextures.find(key);
            if (found != loadedTextures.end()) {
                mesh.diffuseTex = found->second;
            } else {
                GLuint texId = 0;

                if (!key.empty() && key[0] == '*') {
                    const aiTexture* embedded = scene->GetEmbeddedTexture(key.c_str());
                    if (embedded) {
                        if (embedded->mHeight == 0) {
                            texId = loadTextureFromMemory(reinterpret_cast<const unsigned char*>(embedded->pcData), embedded->mWidth);
                        } else {
                            const std::size_t pixelCount = static_cast<std::size_t>(embedded->mWidth) * embedded->mHeight;
                            std::vector<unsigned char> rgba(pixelCount * 4);
                            for (std::size_t p = 0; p < pixelCount; ++p) {
                                rgba[p * 4 + 0] = embedded->pcData[p].r;
                                rgba[p * 4 + 1] = embedded->pcData[p].g;
                                rgba[p * 4 + 2] = embedded->pcData[p].b;
                                rgba[p * 4 + 3] = embedded->pcData[p].a;
                            }
                            texId = loadTextureFromMemory(rgba.data(), rgba.size());
                        }
                    }
                } else {
                    const std::filesystem::path fullPath = modelDir / key;
                    texId = loadTexture(fullPath.string());
                }

                if (texId != 0) {
                    loadedTextures[key] = texId;
                    mesh.diffuseTex = texId;
                }
            }
        }
    }

    return mesh;
}

void processNode(const aiScene* scene,
                 const aiNode* node,
                 const aiMatrix4x4& parentTransform,
                 const std::filesystem::path& modelDir,
                 std::unordered_map<std::string, GLuint>& loadedTextures,
                 Model& model,
                 glm::vec3& bboxMin,
                 glm::vec3& bboxMax)
{
    const aiMatrix4x4 worldTransform = parentTransform * node->mTransformation;

    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        if (shouldSkipRenderableMesh(mesh->mName.C_Str())) {
            continue;
        }
        model.meshes.push_back(buildMesh(scene, mesh, modelDir, worldTransform, model, loadedTextures, bboxMin, bboxMax));
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        processNode(scene, node->mChildren[i], worldTransform, modelDir, loadedTextures, model, bboxMin, bboxMax);
    }
}
}  // namespace

Model loadModel(const std::string& path)
{
    Model model{};
    hasRectorateLayout = false;
    rectorateLayout = RectorateLayout{};

    Assimp::Importer importer;
    // PreTransformVertices flattens node transforms into final vertex positions.
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices);

    if (!scene || !scene->mRootNode || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)) {
        std::cerr << "Assimp load failed for " << path << ": " << importer.GetErrorString() << std::endl;
        return model;
    }

    const std::filesystem::path modelDir = std::filesystem::path(path).parent_path();
    std::unordered_map<std::string, GLuint> loadedTextures;

    glm::vec3 bboxMin(std::numeric_limits<float>::max());
    glm::vec3 bboxMax(std::numeric_limits<float>::lowest());
    processNode(scene, scene->mRootNode, aiMatrix4x4(), modelDir, loadedTextures, model, bboxMin, bboxMax);
    if (isCampusScenePath(path)) {
        appendHenryFordSiteDetails(model, bboxMin, bboxMax);
        appendRectorateFountainDetails(model, bboxMin, bboxMax);
        appendCampusFenceAndForest(model, bboxMin, bboxMax);
    }

    if (!model.meshes.empty()) {
        model.bboxMin = bboxMin;
        model.bboxMax = bboxMax;
        const glm::vec3 extent = model.bboxMax - model.bboxMin;
        model.sceneHasTerrain = (extent.x > 250.0f && extent.z > 250.0f);
    }

    std::size_t totalIndices = 0;
    std::size_t texturedMeshes = 0;
    for (const auto& mesh : model.meshes) {
        totalIndices += static_cast<std::size_t>(mesh.indexCount);
        if (mesh.diffuseTex != 0) texturedMeshes++;
    }
    std::cout << "Model stats: meshes=" << model.meshes.size()
              << ", triangles=" << (totalIndices / 3)
              << ", texturedMeshes=" << texturedMeshes
              << ", walkSurfaceTriangles=" << model.walkSurface.size()
              << ", obstacles=" << model.obstacles.size() << std::endl;

    return model;
}
