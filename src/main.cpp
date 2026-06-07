#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "camera.h"
#include "mesh.h"
#include "shader.h"

// ---------------------------------------------------------------------------
// Window
// ---------------------------------------------------------------------------
const int WIDTH = 1280;
const int HEIGHT = 720;

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
Camera camera(glm::vec3(0.0f, 1.7f, 8.0f));
GLuint shaderProgram = 0;
GLuint groundVAO = 0;
GLuint fallbackWhiteTex = 0;
Model campusModel;
Model odeonModel;
Model clockTowerModel;
glm::mat4 sceneRootTransform(1.0f);
float sceneScale = 1.0f;
float farPlane = 2000.0f;

float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------
float groundVertices[] = {
    // position               // normal      // uv
    -500.0f, 0.0f,  500.0f,   0, 1, 0,      0, 0,
     500.0f, 0.0f,  500.0f,   0, 1, 0,      1, 0,
     500.0f, 0.0f, -500.0f,   0, 1, 0,      1, 1,
     500.0f, 0.0f, -500.0f,   0, 1, 0,      1, 1,
    -500.0f, 0.0f, -500.0f,   0, 1, 0,      0, 1,
    -500.0f, 0.0f,  500.0f,   0, 1, 0,      0, 0,
};

// ---------------------------------------------------------------------------
// Callbacks
// ---------------------------------------------------------------------------
void mouseCallback(GLFWwindow* /*window*/, double xpos, double ypos)
{
    const float xf = static_cast<float>(xpos);
    const float yf = static_cast<float>(ypos);

    if (firstMouse) {
        lastX = xf;
        lastY = yf;
        firstMouse = false;
    }

    camera.processMouse(xf - lastX, lastY - yf);
    lastX = xf;
    lastY = yf;
}

void framebufferSizeCallback(GLFWwindow* /*window*/, int width, int height)
{
    glViewport(0, 0, width, height);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static GLuint createVAO(const float* data, size_t size)
{
    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    glBindVertexArray(0);
    return vao;
}

static void setUniform(GLuint prog, const char* name, const glm::mat4& m)
{
    glUniformMatrix4fv(glGetUniformLocation(prog, name), 1, GL_FALSE, glm::value_ptr(m));
}

static void setUniform(GLuint prog, const char* name, const glm::vec3& v)
{
    glUniform3fv(glGetUniformLocation(prog, name), 1, glm::value_ptr(v));
}

static void setUniform(GLuint prog, const char* name, int value)
{
    glUniform1i(glGetUniformLocation(prog, name), value);
}

struct LandmarkView {
    const char* name;
    glm::vec3 position;
    float yaw;
    float pitch;
};

static const LandmarkView LANDMARKS[] = {
    {"Rectorate", glm::vec3(-2.0f, 112.0f, 188.0f), -82.0f, -13.0f},
    {"Library", glm::vec3(-116.0f, 116.0f, 102.0f), -28.0f, -10.0f},
    {"Student Center", glm::vec3(72.0f, 112.0f, -62.0f), -142.0f, -11.0f},
};

static void jumpToLandmark(int index)
{
    if (index < 0 || index >= static_cast<int>(sizeof(LANDMARKS) / sizeof(LANDMARKS[0]))) {
        return;
    }

    const LandmarkView& view = LANDMARKS[index];
    camera.flyMode = true;
    camera.position = view.position * sceneScale;
    camera.yaw = view.yaw;
    camera.pitch = view.pitch;
    camera.walkHeight = camera.position.y;
    firstMouse = true;
    std::cout << "Landmark view: " << view.name << std::endl;
}

static std::string resolveScenePath()
{
    namespace fs = std::filesystem;
    const std::vector<std::string> candidates = {
        "assets/models/Campus.glb",
        "assets/models/Campus.gltf",
        "assets/models/Campus.obj",
        "assets/models/campus.glb",
        "assets/models/campus.gltf",
        "assets/models/campus.obj",
        "assets/models/object/campus.glb",
        "assets/models/object/3_7_2026.glb",
    };

    for (const auto& p : candidates) {
        if (fs::exists(p)) {
            return p;
        }
    }
    return "";
}

static std::string resolveOdeonPath()
{
    namespace fs = std::filesystem;
    const std::vector<std::string> candidates = {
        "assets/models/object/odeon.obj",
        "assets/models/object/odeon.glb",
        "assets/models/odeon.obj",
        "assets/models/odeon.glb",
    };

    for (const auto& p : candidates) {
        if (fs::exists(p)) {
            return p;
        }
    }
    return "";
}

static std::string resolveClockTowerPath()
{
    namespace fs = std::filesystem;
    const std::vector<std::string> candidates = {
        "assets/models/object/clock_tower.obj",
        "assets/models/object/clock_tower.glb",
        "assets/models/clock_tower.obj",
        "assets/models/clock_tower.glb",
    };

    for (const auto& p : candidates) {
        if (fs::exists(p)) {
            return p;
        }
    }
    return "";
}

static glm::vec3 sceneExtent(const Model& m)
{
    return m.bboxMax - m.bboxMin;
}

static GLuint createFallbackWhiteTexture()
{
    const unsigned char pixel[3] = {255, 255, 255};
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, pixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

static void orientCameraToward(const glm::vec3& target)
{
    const glm::vec3 delta = target - camera.position;
    if (glm::length(delta) < 1e-4f) {
        camera.yaw = -90.0f;
        camera.pitch = 0.0f;
        return;
    }
    const glm::vec3 dir = glm::normalize(delta);
    camera.yaw = glm::degrees(std::atan2(dir.z, dir.x));
    camera.pitch = glm::degrees(std::asin(glm::clamp(dir.y, -1.0f, 1.0f)));
}

static bool sampleWalkSurfaceHeight(float worldX, float worldZ, float maxProbeY, float& outY,
                                    float preferredY = std::numeric_limits<float>::quiet_NaN())
{
    if (campusModel.walkSurface.empty()) {
        return false;
    }

    const float x = worldX / sceneScale;
    const float z = worldZ / sceneScale;
    const float maxY = maxProbeY / sceneScale;

    bool found = false;
    float bestY = -1e9f;
    float bestDiff = std::numeric_limits<float>::max();
    const float eps = 1e-5f;

    for (const auto& tri : campusModel.walkSurface) {
        const float minX = std::min({tri.a.x, tri.b.x, tri.c.x}) - eps;
        const float maxX = std::max({tri.a.x, tri.b.x, tri.c.x}) + eps;
        const float minZ = std::min({tri.a.z, tri.b.z, tri.c.z}) - eps;
        const float maxZ = std::max({tri.a.z, tri.b.z, tri.c.z}) + eps;
        if (x < minX || x > maxX || z < minZ || z > maxZ) {
            continue;
        }

        const glm::vec2 a(tri.a.x, tri.a.z);
        const glm::vec2 b(tri.b.x, tri.b.z);
        const glm::vec2 c(tri.c.x, tri.c.z);
        const glm::vec2 p(x, z);

        const glm::vec2 v0 = b - a;
        const glm::vec2 v1 = c - a;
        const glm::vec2 v2 = p - a;
        const float det = v0.x * v1.y - v1.x * v0.y;
        if (std::abs(det) < eps) {
            continue;
        }

        const float invDet = 1.0f / det;
        const float u = (v2.x * v1.y - v1.x * v2.y) * invDet;
        const float v = (v0.x * v2.y - v2.x * v0.y) * invDet;
        const float w = 1.0f - u - v;
        if (u < -eps || v < -eps || w < -eps) {
            continue;
        }

        const float y = u * tri.b.y + v * tri.c.y + w * tri.a.y;
        if (y <= maxY + eps) {
            if (std::isnan(preferredY)) {
                if (y > bestY) {
                    bestY = y;
                    found = true;
                }
            } else {
                const float yWorld = y * sceneScale;
                const float diff = std::abs(yWorld - preferredY);
                if (diff < bestDiff) {
                    bestDiff = diff;
                    bestY = y;
                    found = true;
                }
            }
        }
    }

    if (found) {
        outY = bestY * sceneScale;
    }
    return found;
}

static void appendWalkData(Model& target, const Model& source)
{
    target.walkSurface.insert(target.walkSurface.end(), source.walkSurface.begin(), source.walkSurface.end());
    target.obstacles.insert(target.obstacles.end(), source.obstacles.begin(), source.obstacles.end());
}

static bool collidesWithBuilding(float worldX, float worldY, float worldZ)
{
    const float x = worldX / sceneScale;
    const float y = worldY / sceneScale;
    const float z = worldZ / sceneScale;
    const float radius = 0.6f / sceneScale;  // ~60 cm body radius

    for (const auto& o : campusModel.obstacles) {
        if (x < o.minXZ.x - radius || x > o.maxXZ.x + radius) continue;
        if (z < o.minXZ.y - radius || z > o.maxXZ.y + radius) continue;
        if (y < o.minY - 2.0f || y > o.maxY + 2.0f) continue;
        return true;
    }
    return false;
}

static void printCameraProbe()
{
    const float maxProbeY = std::max(camera.position.y + 5.0f, campusModel.bboxMax.y * sceneScale + 10.0f);
    float groundY = 0.0f;
    const bool hasGround = sampleWalkSurfaceHeight(camera.position.x, camera.position.z, maxProbeY, groundY);

    std::cout << "Camera probe: eye=("
              << camera.position.x << ", " << camera.position.y << ", " << camera.position.z << ")"
              << " yaw/pitch=(" << camera.yaw << ", " << camera.pitch << ")"
              << " flyMode=" << (camera.flyMode ? "on" : "off");
    if (hasGround) {
        std::cout << " groundY=" << groundY
                  << " foot=(" << camera.position.x << ", " << groundY << ", " << camera.position.z << ")";
    } else {
        std::cout << " groundY=none";
    }
    std::cout << std::endl;
}

static bool projectToWalkable(const glm::vec3& candidate, glm::vec3& corrected)
{
    float terrainY = 0.0f;
    if (!sampleWalkSurfaceHeight(candidate.x, candidate.z, candidate.y + 5.0f, terrainY, candidate.y - 1.7f)) {
        return false;
    }

    corrected = glm::vec3(candidate.x, terrainY + 1.7f, candidate.z);
    const float deltaY = corrected.y - candidate.y;
    if (deltaY > 1.2f || deltaY < -3.5f) {
        return false;
    }
    if (collidesWithBuilding(corrected.x, corrected.y, corrected.z)) {
        return false;
    }
    return true;
}

static bool findNearbyWalkable(const glm::vec3& origin, glm::vec3& outPos)
{
    if (projectToWalkable(origin, outPos)) {
        return true;
    }

    for (int r = 1; r <= 12; ++r) {
        const float radius = 0.75f * static_cast<float>(r);
        const int samples = 16 + r * 4;
        for (int i = 0; i < samples; ++i) {
            const float a = (2.0f * glm::pi<float>() * static_cast<float>(i)) / static_cast<float>(samples);
            const glm::vec3 candidate = origin + glm::vec3(std::cos(a) * radius, 0.0f, std::sin(a) * radius);
            if (projectToWalkable(candidate, outPos)) {
                return true;
            }
        }
    }
    return false;
}

static bool findGuaranteedSpawn(glm::vec3& outWorldPos)
{
    if (campusModel.walkSurface.empty()) return false;

    const glm::vec3 sceneCenterLocal = (campusModel.bboxMin + campusModel.bboxMax) * 0.5f;
    const float searchStep = 15.0f;
    const int rings = 24;
    const float probeY = (campusModel.bboxMin.y + 30.0f) * sceneScale;

    float h = 0.0f;
    if (sampleWalkSurfaceHeight(sceneCenterLocal.x * sceneScale, sceneCenterLocal.z * sceneScale, probeY, h)) {
        outWorldPos = glm::vec3(sceneCenterLocal.x * sceneScale, h, sceneCenterLocal.z * sceneScale);
        return true;
    }

    for (int r = 1; r <= rings; ++r) {
        const float radius = r * searchStep;
        const int samples = 12 + r * 4;
        for (int i = 0; i < samples; ++i) {
            const float a = (2.0f * glm::pi<float>() * static_cast<float>(i)) / static_cast<float>(samples);
            const float x = (sceneCenterLocal.x + std::cos(a) * radius) * sceneScale;
            const float z = (sceneCenterLocal.z + std::sin(a) * radius) * sceneScale;
            if (sampleWalkSurfaceHeight(x, z, probeY, h)) {
                outWorldPos = glm::vec3(x, h, z);
                return true;
            }
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Init & Display
// ---------------------------------------------------------------------------
void init()
{
    shaderProgram = loadShaders("shaders/vshader.glsl", "shaders/fshader.glsl");
    groundVAO = createVAO(groundVertices, sizeof(groundVertices));

    const std::string scenePath = resolveScenePath();
    if (scenePath.empty()) {
        std::cerr << "No scene file found under assets/models/ (*.obj, *.glb, *.gltf)." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    campusModel = loadModel(scenePath);
    if (campusModel.meshes.empty()) {
        std::cerr << "Scene loaded with zero meshes: " << scenePath << std::endl;
        std::exit(EXIT_FAILURE);
    }

    const glm::vec3 extent = sceneExtent(campusModel);
    const float maxExtent = std::max(extent.x, std::max(extent.y, extent.z));
    if (maxExtent > 2000.0f) {
        sceneRootTransform = glm::scale(glm::mat4(1.0f), glm::vec3(0.01f));
        sceneScale = 0.01f;
        std::cout << "Applied root scale 0.01 (centimeters -> meters assumption)." << std::endl;
    }

    const std::string odeonPath = resolveOdeonPath();
    if (!odeonPath.empty()) {
        odeonModel = loadModel(odeonPath);
        if (!odeonModel.meshes.empty()) {
            appendWalkData(campusModel, odeonModel);
            std::cout << "Loaded odeon: " << odeonPath << std::endl;
        } else {
            std::cerr << "Odeon asset had zero meshes: " << odeonPath << std::endl;
        }
    } else {
        std::cerr << "No odeon asset found under assets/models/object/odeon.{obj,glb}." << std::endl;
    }

    const std::string clockTowerPath = resolveClockTowerPath();
    if (!clockTowerPath.empty()) {
        clockTowerModel = loadModel(clockTowerPath);
        if (!clockTowerModel.meshes.empty()) {
            std::cout << "Loaded clock tower: " << clockTowerPath << std::endl;
        } else {
            std::cerr << "Clock tower asset had zero meshes: " << clockTowerPath << std::endl;
        }
    } else {
        std::cerr << "No clock tower asset found under assets/models/object/clock_tower.{obj,glb}." << std::endl;
    }

    const glm::vec3 sceneCenter = (campusModel.bboxMin + campusModel.bboxMax) * 0.5f;
    glm::vec3 spawnPoint(sceneCenter.x, campusModel.bboxMin.y, sceneCenter.z);
    if (!findGuaranteedSpawn(spawnPoint)) {
        spawnPoint = glm::vec3(sceneCenter.x, campusModel.bboxMin.y, sceneCenter.z);
    }

    camera.position = glm::vec3(spawnPoint.x, spawnPoint.y + 1.7f, spawnPoint.z);
    camera.walkHeight = camera.position.y;
    orientCameraToward(glm::vec3(sceneCenter.x * sceneScale, camera.position.y, sceneCenter.z * sceneScale));
    farPlane = std::max(2000.0f, maxExtent * 4.0f);

    float terrainY = 0.0f;
    if (sampleWalkSurfaceHeight(camera.position.x, camera.position.z, camera.position.y + 5.0f, terrainY)) {
        camera.walkHeight = terrainY + 1.7f;
        camera.position.y = camera.walkHeight;
    }

    if (collidesWithBuilding(camera.position.x, camera.position.y, camera.position.z)) {
        const glm::vec3 center = glm::vec3(sceneCenter.x * sceneScale, 0.0f, sceneCenter.z * sceneScale);
        bool relocated = false;
        for (int r = 1; r <= 20 && !relocated; ++r) {
            const float radius = r * 5.0f;
            for (int i = 0; i < 24; ++i) {
                const float a = (2.0f * glm::pi<float>() * static_cast<float>(i)) / 24.0f;
                const float x = center.x + std::cos(a) * radius;
                const float z = center.z + std::sin(a) * radius;
                float h = 0.0f;
                if (!sampleWalkSurfaceHeight(x, z, camera.position.y + 5.0f, h)) continue;
                const float y = h + 1.7f;
                if (!collidesWithBuilding(x, y, z)) {
                    camera.position = glm::vec3(x, y, z);
                    camera.walkHeight = y;
                    relocated = true;
                    break;
                }
            }
        }
    }

    std::cout << "Loaded scene: " << scenePath << std::endl;
    std::cout << "Scene bbox min: (" << campusModel.bboxMin.x << ", " << campusModel.bboxMin.y << ", " << campusModel.bboxMin.z << ")" << std::endl;
    std::cout << "Scene bbox max: (" << campusModel.bboxMax.x << ", " << campusModel.bboxMax.y << ", " << campusModel.bboxMax.z << ")" << std::endl;
    std::cout << "Scene extent: (" << extent.x << ", " << extent.y << ", " << extent.z << ")" << std::endl;
    std::cout << "Scene has terrain heuristic: " << (campusModel.sceneHasTerrain ? "yes" : "no") << std::endl;
    std::cout << "Camera start: (" << camera.position.x << ", " << camera.position.y << ", " << camera.position.z << ")" << std::endl;
    std::cout << "Camera yaw/pitch: (" << camera.yaw << ", " << camera.pitch << ")" << std::endl;

    glEnable(GL_DEPTH_TEST);
    // Pre-linearize the sky color so it matches the gamma-corrected scene output
    glClearColor(powf(0.60f, 2.2f), powf(0.78f, 2.2f), powf(0.88f, 2.2f), 1.0f);
    fallbackWhiteTex = createFallbackWhiteTexture();

    glUseProgram(shaderProgram);
    setUniform(shaderProgram, "diffuseMap", 0);
    setUniform(shaderProgram, "hasTexture", 0);
    setUniform(shaderProgram, "materialMode", 0);
}

static void drawModel(const Model& model)
{
    for (const auto& mesh : model.meshes) {
        if (mesh.diffuseTex != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mesh.diffuseTex);
            setUniform(shaderProgram, "hasTexture", 1);
        } else {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, fallbackWhiteTex);
            setUniform(shaderProgram, "hasTexture", 0);
        }
        setUniform(shaderProgram, "materialMode", mesh.materialMode);
        setUniform(shaderProgram, "objectColor", mesh.baseColor);
        glBindVertexArray(mesh.vao);
        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
    }
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fallbackWhiteTex);

    const glm::mat4 view = camera.viewMatrix();
    const glm::mat4 projection = glm::perspective(glm::radians(60.0f),
                                                  static_cast<float>(WIDTH) / static_cast<float>(HEIGHT),
                                                  0.1f, farPlane);
    setUniform(shaderProgram, "view", view);
    setUniform(shaderProgram, "projection", projection);
    setUniform(shaderProgram, "viewPos", camera.position);
    setUniform(shaderProgram, "lightDir", glm::normalize(glm::vec3(-0.35f, 0.85f, 0.45f)));
    setUniform(shaderProgram, "lightColor", glm::vec3(0.92f, 0.90f, 0.84f));

    if (!campusModel.sceneHasTerrain) {
        setUniform(shaderProgram, "model", glm::mat4(1.0f));
        setUniform(shaderProgram, "hasTexture", 0);
        setUniform(shaderProgram, "materialMode", 0);
        setUniform(shaderProgram, "objectColor", glm::vec3(0.30f, 0.55f, 0.25f));
        glBindVertexArray(groundVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    setUniform(shaderProgram, "model", sceneRootTransform);
    drawModel(campusModel);
    drawModel(odeonModel);
    drawModel(clockTowerModel);
    glBindVertexArray(0);
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------
static bool flyKeyWasPressed = false;
static bool probeKeyWasPressed = false;
static bool landmarkKeyWasPressed[3] = {false, false, false};

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // Toggle fly mode with F
    const bool flyKeyDown = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;
    if (flyKeyDown && !flyKeyWasPressed) {
        camera.flyMode = !camera.flyMode;
        std::cout << (camera.flyMode ? "Fly mode ON" : "Fly mode OFF") << std::endl;
    }
    flyKeyWasPressed = flyKeyDown;

    const bool probeKeyDown = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
    if (probeKeyDown && !probeKeyWasPressed) {
        printCameraProbe();
    }
    probeKeyWasPressed = probeKeyDown;

    const int landmarkKeys[3] = {GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3};
    for (int i = 0; i < 3; ++i) {
        const bool keyDown = glfwGetKey(window, landmarkKeys[i]) == GLFW_PRESS;
        if (keyDown && !landmarkKeyWasPressed[i]) {
            jumpToLandmark(i);
        }
        landmarkKeyWasPressed[i] = keyDown;
    }

    if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
        camera.speed = camera.flyMode ? 80.0f : 35.0f;
    } else if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        camera.speed = camera.flyMode ? 40.0f : 15.0f;
    } else {
        camera.speed = camera.flyMode ? 20.0f : 5.0f;
    }

    if (camera.flyMode) {
        // Fly mode: free movement along camera direction, no collision
        glm::vec3 move(0.0f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) move += camera.front();
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) move -= camera.front();
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) move -= camera.right();
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) move += camera.right();
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) move.y += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)     move.y -= 1.0f;

        if (glm::length(move) > 1e-5f) {
            camera.position += glm::normalize(move) * (camera.speed * deltaTime);
        }
        return;
    }

    // Walk mode: terrain following + collision
    const glm::vec3 prevPos = camera.position;
    glm::vec3 move(0.0f);

    glm::vec3 flatFront = glm::vec3(camera.front().x, 0.0f, camera.front().z);
    if (glm::length(flatFront) > 1e-5f) {
        flatFront = glm::normalize(flatFront);
    }
    glm::vec3 flatRight = glm::vec3(camera.right().x, 0.0f, camera.right().z);
    if (glm::length(flatRight) > 1e-5f) {
        flatRight = glm::normalize(flatRight);
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) move += flatFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) move -= flatFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) move -= flatRight;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) move += flatRight;

    glm::vec3 resolvedPos = prevPos;
    if (glm::length(move) > 1e-5f) {
        move = glm::normalize(move) * (camera.speed * deltaTime);
        glm::vec3 corrected;

        const glm::vec3 fullTarget = prevPos + move;
        if (projectToWalkable(fullTarget, corrected)) {
            resolvedPos = corrected;
        } else {
            bool moved = false;
            const glm::vec3 xTarget = prevPos + glm::vec3(move.x, 0.0f, 0.0f);
            if (projectToWalkable(xTarget, corrected)) {
                resolvedPos = corrected;
                moved = true;
            }

            const glm::vec3 zTarget = prevPos + glm::vec3(0.0f, 0.0f, move.z);
            if (projectToWalkable(zTarget, corrected)) {
                resolvedPos = corrected;
                moved = true;
            }

            if (!moved) {
                glm::vec3 unstuck;
                if (findNearbyWalkable(prevPos, unstuck)) {
                    resolvedPos = unstuck;
                }
            }
        }
    } else {
        glm::vec3 corrected;
        if (projectToWalkable(prevPos, corrected)) {
            resolvedPos = corrected;
        }
    }

    camera.position = resolvedPos;
    camera.walkHeight = camera.position.y;
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

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to init GLEW" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

    std::cout << "OpenGL " << glGetString(GL_VERSION) << std::endl;
    std::cout << "Controls: WASD = move, Mouse = look, Shift = sprint, Alt = turbo, F = fly mode, Space/C = up/down (fly), P = print camera probe, 1/2/3 = landmarks, ESC = quit" << std::endl;

    init();

    while (!glfwWindowShouldClose(window)) {
        const float currentFrame = static_cast<float>(glfwGetTime());
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
