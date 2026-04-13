#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <limits>

#include "shader.h"
#include "camera.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

// ---------------------------------------------------------------------------
// Window
// ---------------------------------------------------------------------------
const int WIDTH  = 1280;
const int HEIGHT = 720;

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
Camera camera(glm::vec3(0.0f, 1.7f, 80.0f));  // start south of campus, looking north
GLuint shaderProgram;
GLuint cubeVAO, cubeVBO;
GLuint groundVAO, groundVBO;
std::unordered_map<std::string, GLuint> buildingTextures;

struct MeshPrimitive {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLsizei vertexCount = 0;
    GLuint texture = 0;
    glm::vec3 baseColor = glm::vec3(1.0f);
};
std::vector<MeshPrimitive> scanModelPrimitives;
bool scanModelLoaded = false;
glm::vec3 scanModelCenter(0.0f);
glm::vec3 scanModelMin(0.0f);
float scanModelScale = 1.0f;

float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
bool  firstMouse = true;
float deltaTime  = 0.0f;
float lastFrame  = 0.0f;

// ---------------------------------------------------------------------------
// Building definition
// ---------------------------------------------------------------------------
struct Building {
    std::string name;
    glm::vec3 position;  // center of base
    glm::vec3 size;      // width (X), height (Y), depth (Z)
    glm::vec3 color;
};

// Generated from OpenStreetMap data (scripts/parse_osm.py)
// Reference: lat=41.2058, lon=29.0740
// X = East, Z = South (positive = south), Y = Up
std::vector<Building> buildings = {
    {"Auditorium",     {  28.3f, 0.0f,  -66.2f}, { 56.2f, 10.0f,  55.1f}, {0.72f, 0.68f, 0.65f}},
    {"CASE",           {   7.0f, 0.0f,  -19.2f}, { 70.7f, 16.0f,  70.6f}, {0.70f, 0.72f, 0.68f}},
    {"CASS",           {  81.6f, 0.0f,  -35.9f}, { 55.9f, 16.0f,  76.4f}, {0.73f, 0.71f, 0.68f}},
    {"ELC",            { -59.1f, 0.0f,  -43.2f}, { 37.6f, 12.0f,  30.2f}, {0.74f, 0.72f, 0.69f}},
    {"ENG",            { 130.9f, 0.0f, -141.1f}, { 96.8f, 16.0f,  91.9f}, {0.75f, 0.70f, 0.65f}},
    {"Library",        { -57.3f, 0.0f,   -0.5f}, { 42.2f, 12.0f,  51.6f}, {0.85f, 0.80f, 0.72f}},
    {"MED",            {  73.9f, 0.0f, -118.8f}, { 50.6f, 16.0f,  49.8f}, {0.77f, 0.73f, 0.70f}},
    {"Rectorate",      { -93.9f, 0.0f,   53.4f}, { 22.5f, 10.0f,  61.7f}, {0.80f, 0.78f, 0.72f}},
    {"SCI",            { 118.3f, 0.0f,  -83.5f}, { 55.0f, 16.0f,  76.0f}, {0.80f, 0.75f, 0.70f}},
    {"SNA",            { 112.9f, 0.0f, -235.1f}, { 95.3f, 16.0f, 105.1f}, {0.76f, 0.74f, 0.71f}},
    {"Sports",         {-124.0f, 0.0f, -178.1f}, { 65.3f, 14.0f,  73.3f}, {0.65f, 0.70f, 0.75f}},
    {"StudentCenter",  { -50.8f, 0.0f,  109.6f}, { 69.2f, 10.0f,  68.7f}, {0.78f, 0.74f, 0.70f}},
};

// ---------------------------------------------------------------------------
// Geometry: unit cube (centered at origin, size 1x1x1) with normals
// ---------------------------------------------------------------------------
// Each face: 2 triangles, 6 vertices. 6 faces = 36 vertices.
// Per vertex: position (3) + normal (3) + UV (2) = 8 floats.

// clang-format off
float cubeVertices[] = {
    // positions          // normals      // uv
    // Front face (+Z)
    -0.5f, 0.0f,  0.5f,   0, 0, 1,   0, 0,
     0.5f, 0.0f,  0.5f,   0, 0, 1,   1, 0,
     0.5f, 1.0f,  0.5f,   0, 0, 1,   1, 1,
     0.5f, 1.0f,  0.5f,   0, 0, 1,   1, 1,
    -0.5f, 1.0f,  0.5f,   0, 0, 1,   0, 1,
    -0.5f, 0.0f,  0.5f,   0, 0, 1,   0, 0,
    // Back face (-Z)
     0.5f, 0.0f, -0.5f,   0, 0,-1,   0, 0,
    -0.5f, 0.0f, -0.5f,   0, 0,-1,   1, 0,
    -0.5f, 1.0f, -0.5f,   0, 0,-1,   1, 1,
    -0.5f, 1.0f, -0.5f,   0, 0,-1,   1, 1,
     0.5f, 1.0f, -0.5f,   0, 0,-1,   0, 1,
     0.5f, 0.0f, -0.5f,   0, 0,-1,   0, 0,
    // Left face (-X)
    -0.5f, 0.0f, -0.5f,  -1, 0, 0,   0, 0,
    -0.5f, 0.0f,  0.5f,  -1, 0, 0,   1, 0,
    -0.5f, 1.0f,  0.5f,  -1, 0, 0,   1, 1,
    -0.5f, 1.0f,  0.5f,  -1, 0, 0,   1, 1,
    -0.5f, 1.0f, -0.5f,  -1, 0, 0,   0, 1,
    -0.5f, 0.0f, -0.5f,  -1, 0, 0,   0, 0,
    // Right face (+X)
     0.5f, 0.0f,  0.5f,   1, 0, 0,   0, 0,
     0.5f, 0.0f, -0.5f,   1, 0, 0,   1, 0,
     0.5f, 1.0f, -0.5f,   1, 0, 0,   1, 1,
     0.5f, 1.0f, -0.5f,   1, 0, 0,   1, 1,
     0.5f, 1.0f,  0.5f,   1, 0, 0,   0, 1,
     0.5f, 0.0f,  0.5f,   1, 0, 0,   0, 0,
    // Top face (+Y)
    -0.5f, 1.0f,  0.5f,   0, 1, 0,   0, 0,
     0.5f, 1.0f,  0.5f,   0, 1, 0,   1, 0,
     0.5f, 1.0f, -0.5f,   0, 1, 0,   1, 1,
     0.5f, 1.0f, -0.5f,   0, 1, 0,   1, 1,
    -0.5f, 1.0f, -0.5f,   0, 1, 0,   0, 1,
    -0.5f, 1.0f,  0.5f,   0, 1, 0,   0, 0,
    // Bottom face (-Y)
    -0.5f, 0.0f, -0.5f,   0,-1, 0,   0, 0,
     0.5f, 0.0f, -0.5f,   0,-1, 0,   1, 0,
     0.5f, 0.0f,  0.5f,   0,-1, 0,   1, 1,
     0.5f, 0.0f,  0.5f,   0,-1, 0,   1, 1,
    -0.5f, 0.0f,  0.5f,   0,-1, 0,   0, 1,
    -0.5f, 0.0f, -0.5f,   0,-1, 0,   0, 0,
};
// clang-format on

// Ground plane: large quad at y = 0
float groundVertices[] = {
    // positions              // normals    // uv
    -500.0f, 0.0f,  500.0f,   0, 1, 0,     0, 0,
     500.0f, 0.0f,  500.0f,   0, 1, 0,     1, 0,
     500.0f, 0.0f, -500.0f,   0, 1, 0,     1, 1,
     500.0f, 0.0f, -500.0f,   0, 1, 0,     1, 1,
    -500.0f, 0.0f, -500.0f,   0, 1, 0,     0, 1,
    -500.0f, 0.0f,  500.0f,   0, 1, 0,     0, 0,
};

// ---------------------------------------------------------------------------
// Callbacks
// ---------------------------------------------------------------------------

void mouseCallback(GLFWwindow* /*window*/, double xpos, double ypos)
{
    float xf = static_cast<float>(xpos);
    float yf = static_cast<float>(ypos);

    if (firstMouse) {
        lastX = xf;
        lastY = yf;
        firstMouse = false;
    }

    camera.processMouse(xf - lastX, lastY - yf);  // y inverted
    lastX = xf;
    lastY = yf;
}

void framebufferSizeCallback(GLFWwindow* /*window*/, int width, int height)
{
    glViewport(0, 0, width, height);
}

// ---------------------------------------------------------------------------
// Setup helpers
// ---------------------------------------------------------------------------

static GLuint createVAO(const float* data, size_t size)
{
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);

    // position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    // normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    // uv
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    glBindVertexArray(0);
    return vao;
}

static void setUniform(GLuint prog, const char* name, const glm::mat4& m) {
    glUniformMatrix4fv(glGetUniformLocation(prog, name), 1, GL_FALSE, glm::value_ptr(m));
}

static void setUniform(GLuint prog, const char* name, const glm::vec3& v) {
    glUniform3fv(glGetUniformLocation(prog, name), 1, glm::value_ptr(v));
}

static void setUniform(GLuint prog, const char* name, int value) {
    glUniform1i(glGetUniformLocation(prog, name), value);
}

static void setUniform(GLuint prog, const char* name, float value) {
    glUniform1f(glGetUniformLocation(prog, name), value);
}

static GLuint loadTexture2D(const std::string& path)
{
    int width = 0, height = 0, channels = 0;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        return 0;
    }

    GLenum format = GL_RGB;
    GLenum internalFormat = GL_SRGB;
    if (channels == 1) { format = GL_RED; internalFormat = GL_RED; }
    if (channels == 3) { format = GL_RGB; internalFormat = GL_SRGB; }
    if (channels == 4) { format = GL_RGBA; internalFormat = GL_SRGB_ALPHA; }

    GLuint textureID = 0;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return textureID;
}

static GLuint loadTexture2DFromMemory(const unsigned char* bytes, int byteCount)
{
    int width = 0, height = 0, channels = 0;
    unsigned char* data = stbi_load_from_memory(bytes, byteCount, &width, &height, &channels, 0);
    if (!data) {
        return 0;
    }

    GLenum format = GL_RGB;
    GLenum internalFormat = GL_SRGB;
    if (channels == 1) { format = GL_RED; internalFormat = GL_RED; }
    if (channels == 3) { format = GL_RGB; internalFormat = GL_SRGB; }
    if (channels == 4) { format = GL_RGBA; internalFormat = GL_SRGB_ALPHA; }

    GLuint textureID = 0;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return textureID;
}

static std::string toLower(const std::string& s)
{
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

static std::string findBuildingTextureByKeyword(const std::vector<std::string>& keywords)
{
    namespace fs = std::filesystem;
    const fs::path baseDir("assets/textures/buildings");
    if (!fs::exists(baseDir) || !fs::is_directory(baseDir)) {
        return "";
    }

    for (const auto& entry : fs::directory_iterator(baseDir)) {
        if (!entry.is_regular_file()) continue;
        const std::string ext = toLower(entry.path().extension().string());
        if (ext != ".png" && ext != ".jpg" && ext != ".jpeg") continue;

        const std::string stem = toLower(entry.path().stem().string());
        bool allMatch = true;
        for (const auto& keyword : keywords) {
            if (stem.find(toLower(keyword)) == std::string::npos) {
                allMatch = false;
                break;
            }
        }
        if (allMatch) {
            return entry.path().string();
        }
    }
    return "";
}

static const cgltf_accessor* getAccessor(const cgltf_primitive& primitive, cgltf_attribute_type type)
{
    for (cgltf_size i = 0; i < primitive.attributes_count; ++i) {
        if (primitive.attributes[i].type == type) {
            return primitive.attributes[i].data;
        }
    }
    return nullptr;
}

static glm::mat4 toGlmMat4(const cgltf_float* m)
{
    glm::mat4 out(1.0f);
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            out[col][row] = m[row * 4 + col];
        }
    }
    return out;
}

static bool loadScanModelGLB(const std::string& path)
{
    cgltf_options options = {};
    cgltf_data* data = nullptr;

    if (cgltf_parse_file(&options, path.c_str(), &data) != cgltf_result_success) {
        std::cerr << "Failed to parse GLB: " << path << std::endl;
        return false;
    }
    if (cgltf_load_buffers(&options, data, path.c_str()) != cgltf_result_success) {
        std::cerr << "Failed to load GLB buffers: " << path << std::endl;
        cgltf_free(data);
        return false;
    }
    if (cgltf_validate(data) != cgltf_result_success) {
        std::cerr << "GLB validation failed: " << path << std::endl;
        cgltf_free(data);
        return false;
    }

    glm::vec3 minPos(std::numeric_limits<float>::max());
    glm::vec3 maxPos(std::numeric_limits<float>::lowest());
    int primitiveCount = 0;
    std::unordered_map<const cgltf_texture*, GLuint> gltfTextures;
    stbi_set_flip_vertically_on_load(false);

    for (cgltf_size nodeIndex = 0; nodeIndex < data->nodes_count; ++nodeIndex) {
        const cgltf_node* node = &data->nodes[nodeIndex];
        if (!node->mesh) continue;

        cgltf_float nodeWorld[16];
        cgltf_node_transform_world(node, nodeWorld);
        const glm::mat4 world = toGlmMat4(nodeWorld);
        const glm::mat3 normalWorld = glm::mat3(glm::transpose(glm::inverse(world)));

        for (cgltf_size primitiveIndex = 0; primitiveIndex < node->mesh->primitives_count; ++primitiveIndex) {
            const cgltf_primitive& primitive = node->mesh->primitives[primitiveIndex];
            if (primitive.type != cgltf_primitive_type_triangles) continue;

            const cgltf_accessor* posAccessor = getAccessor(primitive, cgltf_attribute_type_position);
            if (!posAccessor) continue;
            const cgltf_accessor* normalAccessor = getAccessor(primitive, cgltf_attribute_type_normal);
            const cgltf_accessor* uvAccessor = getAccessor(primitive, cgltf_attribute_type_texcoord);

            const cgltf_accessor* indexAccessor = primitive.indices;
            const cgltf_size vertexCount = indexAccessor ? indexAccessor->count : posAccessor->count;
            if (vertexCount == 0) continue;

            std::vector<glm::vec3> smoothNormals;
            if (!normalAccessor) {
                smoothNormals.assign(posAccessor->count, glm::vec3(0.0f));
                for (cgltf_size i = 0; i + 2 < vertexCount; i += 3) {
                    const cgltf_size i0 = indexAccessor ? cgltf_accessor_read_index(indexAccessor, i) : i;
                    const cgltf_size i1 = indexAccessor ? cgltf_accessor_read_index(indexAccessor, i + 1) : (i + 1);
                    const cgltf_size i2 = indexAccessor ? cgltf_accessor_read_index(indexAccessor, i + 2) : (i + 2);

                    cgltf_float p0[3] = {0.0f, 0.0f, 0.0f};
                    cgltf_float p1[3] = {0.0f, 0.0f, 0.0f};
                    cgltf_float p2[3] = {0.0f, 0.0f, 0.0f};
                    cgltf_accessor_read_float(posAccessor, i0, p0, 3);
                    cgltf_accessor_read_float(posAccessor, i1, p1, 3);
                    cgltf_accessor_read_float(posAccessor, i2, p2, 3);

                    const glm::vec3 v0(static_cast<float>(p0[0]), static_cast<float>(p0[1]), static_cast<float>(p0[2]));
                    const glm::vec3 v1(static_cast<float>(p1[0]), static_cast<float>(p1[1]), static_cast<float>(p1[2]));
                    const glm::vec3 v2(static_cast<float>(p2[0]), static_cast<float>(p2[1]), static_cast<float>(p2[2]));
                    const glm::vec3 faceNormal = glm::cross(v1 - v0, v2 - v0);

                    smoothNormals[i0] += faceNormal;
                    smoothNormals[i1] += faceNormal;
                    smoothNormals[i2] += faceNormal;
                }
                for (auto& n : smoothNormals) {
                    const float len2 = glm::dot(n, n);
                    if (len2 > 1e-12f) n = glm::normalize(n);
                    else n = glm::vec3(0.0f, 1.0f, 0.0f);
                }
            }

            std::vector<float> vertices;
            vertices.reserve(vertexCount * 8);
            for (cgltf_size i = 0; i + 2 < vertexCount; i += 3) {
                glm::vec3 triPos[3];
                glm::vec3 triNormal[3];
                glm::vec2 triUV[3];

                for (int k = 0; k < 3; ++k) {
                    const cgltf_size sourceIndex = indexAccessor ? cgltf_accessor_read_index(indexAccessor, i + k) : (i + k);

                    cgltf_float p[3] = {0.0f, 0.0f, 0.0f};
                    cgltf_accessor_read_float(posAccessor, sourceIndex, p, 3);
                    const glm::vec4 worldPos = world * glm::vec4(static_cast<float>(p[0]), static_cast<float>(p[1]), static_cast<float>(p[2]), 1.0f);
                    triPos[k] = glm::vec3(worldPos);

                    triNormal[k] = glm::vec3(0.0f);
                    if (normalAccessor) {
                        cgltf_float nn[3] = {0.0f, 1.0f, 0.0f};
                        cgltf_accessor_read_float(normalAccessor, sourceIndex, nn, 3);
                        triNormal[k] = glm::normalize(
                            normalWorld * glm::vec3(static_cast<float>(nn[0]), static_cast<float>(nn[1]), static_cast<float>(nn[2])));
                    } else if (sourceIndex < smoothNormals.size()) {
                        triNormal[k] = glm::normalize(normalWorld * smoothNormals[sourceIndex]);
                    }

                    triUV[k] = glm::vec2(0.0f, 0.0f);
                    if (uvAccessor) {
                        cgltf_float tt[2] = {0.0f, 0.0f};
                        cgltf_accessor_read_float(uvAccessor, sourceIndex, tt, 2);
                        triUV[k] = glm::vec2(static_cast<float>(tt[0]), static_cast<float>(tt[1]));
                    }

                    minPos = glm::min(minPos, triPos[k]);
                    maxPos = glm::max(maxPos, triPos[k]);
                }

                for (int k = 0; k < 3; ++k) {
                    vertices.push_back(triPos[k].x);
                    vertices.push_back(triPos[k].y);
                    vertices.push_back(triPos[k].z);
                    vertices.push_back(triNormal[k].x);
                    vertices.push_back(triNormal[k].y);
                    vertices.push_back(triNormal[k].z);
                    vertices.push_back(triUV[k].x);
                    vertices.push_back(triUV[k].y);
                }
            }

            MeshPrimitive meshPrim;
            glGenVertexArrays(1, &meshPrim.vao);
            glGenBuffers(1, &meshPrim.vbo);

            glBindVertexArray(meshPrim.vao);
            glBindBuffer(GL_ARRAY_BUFFER, meshPrim.vbo);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

            glBindVertexArray(0);
            meshPrim.vertexCount = static_cast<GLsizei>(vertices.size() / 8);

            if (primitive.material && primitive.material->has_pbr_metallic_roughness) {
                const cgltf_pbr_metallic_roughness& pbr = primitive.material->pbr_metallic_roughness;
                meshPrim.baseColor = glm::vec3(pbr.base_color_factor[0], pbr.base_color_factor[1], pbr.base_color_factor[2]);

                const cgltf_texture* tex = pbr.base_color_texture.texture;
                if (tex) {
                    auto texIt = gltfTextures.find(tex);
                    if (texIt != gltfTextures.end()) {
                        meshPrim.texture = texIt->second;
                    } else if (tex->image) {
                        GLuint texId = 0;
                        if (tex->image->buffer_view && tex->image->buffer_view->buffer && tex->image->buffer_view->buffer->data) {
                            const auto* bytes = static_cast<const unsigned char*>(tex->image->buffer_view->buffer->data) +
                                                tex->image->buffer_view->offset;
                            const int byteCount = static_cast<int>(tex->image->buffer_view->size);
                            texId = loadTexture2DFromMemory(bytes, byteCount);
                        } else if (tex->image->uri) {
                            namespace fs = std::filesystem;
                            const fs::path imagePath = fs::path(path).parent_path() / tex->image->uri;
                            texId = loadTexture2D(imagePath.string());
                        }
                        if (texId != 0) {
                            gltfTextures[tex] = texId;
                            meshPrim.texture = texId;
                        }
                    }
                }
            }

            scanModelPrimitives.push_back(meshPrim);
            primitiveCount++;
        }
    }

    stbi_set_flip_vertically_on_load(true);
    cgltf_free(data);
    if (scanModelPrimitives.empty()) {
        std::cerr << "No triangle primitives found in GLB: " << path << std::endl;
        return false;
    }

    const glm::vec3 size = maxPos - minPos;
    const float maxDimension = std::max(size.x, std::max(size.y, size.z));
    scanModelCenter = (minPos + maxPos) * 0.5f;
    scanModelMin = minPos;
    scanModelScale = (maxDimension > 0.0f) ? (25.0f / maxDimension) : 1.0f;
    scanModelLoaded = true;

    int texturedPrimitives = 0;
    for (const auto& p : scanModelPrimitives) {
        if (p.texture != 0) texturedPrimitives++;
    }
    std::cout << "Loaded scan model: " << primitiveCount << " primitives, "
              << texturedPrimitives << " textured" << std::endl;
    return true;
}

// ---------------------------------------------------------------------------
// Init & Display
// ---------------------------------------------------------------------------

void init()
{
    shaderProgram = loadShaders("shaders/vshader.glsl", "shaders/fshader.glsl");
    cubeVAO   = createVAO(cubeVertices,   sizeof(cubeVertices));
    groundVAO = createVAO(groundVertices, sizeof(groundVertices));

    stbi_set_flip_vertically_on_load(true);
    const std::string libraryTex = findBuildingTextureByKeyword({"library"});
    const std::string rectorateTex = findBuildingTextureByKeyword({"rectorate"});
    const std::string studentCenterTex = findBuildingTextureByKeyword({"student", "center"});

    if (!libraryTex.empty()) {
        buildingTextures["Library"] = loadTexture2D(libraryTex);
        std::cout << "Loaded Library texture: " << libraryTex << std::endl;
    }
    if (!rectorateTex.empty()) {
        buildingTextures["Rectorate"] = loadTexture2D(rectorateTex);
        std::cout << "Loaded Rectorate texture: " << rectorateTex << std::endl;
    }
    if (!studentCenterTex.empty()) {
        buildingTextures["StudentCenter"] = loadTexture2D(studentCenterTex);
        std::cout << "Loaded StudentCenter texture: " << studentCenterTex << std::endl;
    }
    loadScanModelGLB("assets/models/object/3_7_2026.glb");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);  // sky blue

    glUseProgram(shaderProgram);
    setUniform(shaderProgram, "buildingTex", 0);
    setUniform(shaderProgram, "useTexture", 0);
    setUniform(shaderProgram, "ambientStrength", 0.15f);
    setUniform(shaderProgram, "specularStrength", 0.4f);
    setUniform(shaderProgram, "shininess", 64.0f);
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), GL_FALSE);

    // Camera matrices
    glm::mat4 view = camera.viewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(60.0f),
        (float)WIDTH / (float)HEIGHT, 0.1f, 1000.0f);

    setUniform(shaderProgram, "view", view);
    setUniform(shaderProgram, "projection", projection);
    setUniform(shaderProgram, "viewPos", camera.position);

    // Sun light — coming from upper-right
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));
    setUniform(shaderProgram, "lightDir",   lightDir);
    setUniform(shaderProgram, "lightColor", glm::vec3(1.0f, 0.98f, 0.95f));

    // Draw ground
    {
        glm::mat4 model(1.0f);
        setUniform(shaderProgram, "model", model);
        setUniform(shaderProgram, "ambientStrength", 0.15f);
        setUniform(shaderProgram, "specularStrength", 0.10f);
        setUniform(shaderProgram, "shininess", 32.0f);
        setUniform(shaderProgram, "useTexture", 0);
        setUniform(shaderProgram, "objectColor", glm::vec3(0.30f, 0.55f, 0.25f));  // grass green
        glBindVertexArray(groundVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    // Draw buildings
    glBindVertexArray(cubeVAO);
    for (const auto& b : buildings) {
        glm::mat4 model(1.0f);
        model = glm::translate(model, b.position);
        model = glm::scale(model, b.size);
        setUniform(shaderProgram, "ambientStrength", 0.15f);
        setUniform(shaderProgram, "specularStrength", 0.4f);
        setUniform(shaderProgram, "shininess", 64.0f);

        const auto texIt = buildingTextures.find(b.name);
        const bool hasTexture = (texIt != buildingTextures.end() && texIt->second != 0);
        if (hasTexture) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texIt->second);
            setUniform(shaderProgram, "useTexture", 1);
            setUniform(shaderProgram, "objectColor", glm::vec3(1.0f));  // avoid tinting facade textures
        } else {
            setUniform(shaderProgram, "useTexture", 0);
            setUniform(shaderProgram, "objectColor", b.color);
        }

        setUniform(shaderProgram, "model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    // Draw scanned interior model from Polycam GLB
    if (scanModelLoaded) {
        glm::mat4 model(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 30.0f));
        model = glm::scale(model, glm::vec3(scanModelScale));
        model = glm::translate(model, glm::vec3(-scanModelCenter.x, -scanModelMin.y, -scanModelCenter.z));

        setUniform(shaderProgram, "model", model);
        setUniform(shaderProgram, "ambientStrength", 0.34f);
        setUniform(shaderProgram, "specularStrength", 0.08f);
        setUniform(shaderProgram, "shininess", 16.0f);

        for (const auto& prim : scanModelPrimitives) {
            if (prim.texture != 0) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, prim.texture);
                setUniform(shaderProgram, "useTexture", 1);
            } else {
                setUniform(shaderProgram, "useTexture", 0);
            }
            setUniform(shaderProgram, "objectColor", prim.baseColor);
            glBindVertexArray(prim.vao);
            glDrawArrays(GL_TRIANGLES, 0, prim.vertexCount);
        }
    }
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.processKeyboard(0, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.processKeyboard(1, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.processKeyboard(2, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.processKeyboard(3, deltaTime);

    // Sprint with Shift
    camera.speed = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ? 15.0f : 5.0f;
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main()
{
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW" << std::endl;
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "KU Campus Tour", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);

    // Capture mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to init GLEW" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "OpenGL " << glGetString(GL_VERSION) << std::endl;
    std::cout << "Controls: WASD = move, Mouse = look, Shift = sprint, ESC = quit" << std::endl;

    init();

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        display();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return EXIT_SUCCESS;
}
