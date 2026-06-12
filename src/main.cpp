#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <array>
#include <cctype>
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
GLuint skyboxProgram = 0;
GLuint overlayProgram = 0;
GLuint groundVAO = 0;
GLuint skyboxVAO = 0;
GLuint hudVAO = 0;
GLuint hudVBO = 0;
GLuint fallbackWhiteTex = 0;
GLuint skyboxCubemap = 0;
Model campusModel;
Model odeonModel;
Model clockTowerModel;
glm::mat4 sceneRootTransform(1.0f);
float sceneScale = 1.0f;
float farPlane = 2000.0f;
int framebufferWidth = WIDTH;
int framebufferHeight = HEIGHT;
bool nightMode = false;
bool showMinimap = true;
std::size_t lastVisibleMeshes = 0;
std::size_t lastCulledMeshes = 0;

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

float skyboxVertices[] = {
    -1.0f,  1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,   1.0f,  1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,  -1.0f, -1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,   1.0f, -1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,   1.0f,  1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,   1.0f, -1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,   1.0f,  1.0f, -1.0f,   1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f,  1.0f
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
    framebufferWidth = std::max(width, 1);
    framebufferHeight = std::max(height, 1);
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

static GLuint createPositionOnlyVAO(const float* data, size_t size)
{
    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
    return vao;
}

static GLuint createDynamicHudBuffer()
{
    glGenVertexArrays(1, &hudVAO);
    glGenBuffers(1, &hudVBO);
    glBindVertexArray(hudVAO);
    glBindBuffer(GL_ARRAY_BUFFER, hudVBO);
    glBufferData(GL_ARRAY_BUFFER, 4096 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);
    return hudVAO;
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

static void setUniform(GLuint prog, const char* name, float value)
{
    glUniform1f(glGetUniformLocation(prog, name), value);
}

struct LandmarkView {
    const char* name;
    const char* shortLabel;
    glm::vec3 position;
    float yaw;
    float pitch;
    glm::vec3 color;
};

static const LandmarkView LANDMARKS[] = {
    {"Rectorate", "REC", glm::vec3(-2.0f, 112.0f, 188.0f), -82.0f, -13.0f, glm::vec3(0.95f, 0.77f, 0.31f)},
    {"Library", "LIB", glm::vec3(-116.0f, 116.0f, 102.0f), -28.0f, -10.0f, glm::vec3(0.38f, 0.78f, 0.96f)},
    {"Student Center", "SC", glm::vec3(72.0f, 112.0f, -62.0f), -142.0f, -11.0f, glm::vec3(0.94f, 0.52f, 0.34f)},
};

struct FrustumPlane {
    glm::vec3 normal;
    float d = 0.0f;
};

struct LightPreset {
    glm::vec3 direction;
    glm::vec3 color;
    glm::vec3 ambientTint;
    glm::vec3 skyTint;
};

static const LightPreset DAY_PRESET = {
    glm::normalize(glm::vec3(-0.35f, 0.85f, 0.45f)),
    glm::vec3(1.00f, 0.96f, 0.88f),
    glm::vec3(0.86f, 0.88f, 0.84f),
    glm::vec3(0.68f, 0.84f, 0.95f)
};

static const LightPreset NIGHT_PRESET = {
    glm::normalize(glm::vec3(0.28f, 0.42f, -0.86f)),
    glm::vec3(0.26f, 0.34f, 0.48f),
    glm::vec3(0.34f, 0.39f, 0.50f),
    glm::vec3(0.05f, 0.08f, 0.15f)
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

static GLuint createProceduralSkyboxCubemap()
{
    auto encode = [](float v) -> unsigned char {
        const float srgb = std::pow(glm::clamp(v, 0.0f, 1.0f), 1.0f / 2.2f);
        return static_cast<unsigned char>(glm::clamp(srgb * 255.0f, 0.0f, 255.0f));
    };

    const int size = 64;
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex);

    const std::array<glm::vec3, 6> faceDirs = {{
        glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f)
    }};
    const std::array<glm::vec3, 6> faceUps = {{
        glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f),
        glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)
    }};

    for (int face = 0; face < 6; ++face) {
        std::vector<unsigned char> pixels(size * size * 3);
        const glm::vec3 forward = faceDirs[face];
        const glm::vec3 up = faceUps[face];
        const glm::vec3 right = glm::normalize(glm::cross(forward, up));

        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                const float u = (2.0f * (static_cast<float>(x) + 0.5f) / static_cast<float>(size)) - 1.0f;
                const float v = (2.0f * (static_cast<float>(y) + 0.5f) / static_cast<float>(size)) - 1.0f;
                const glm::vec3 dir = glm::normalize(forward + right * u + up * v);

                const float horizon = glm::clamp(dir.y * 0.5f + 0.5f, 0.0f, 1.0f);
                glm::vec3 dayColor = glm::mix(glm::vec3(0.78f, 0.84f, 0.91f),
                                              glm::vec3(0.22f, 0.48f, 0.92f),
                                              std::pow(horizon, 0.65f));
                dayColor += glm::vec3(0.30f, 0.20f, 0.08f) *
                            std::pow(glm::max(glm::dot(dir, DAY_PRESET.direction), 0.0f), 96.0f);

                glm::vec3 nightColor = glm::mix(glm::vec3(0.02f, 0.03f, 0.08f),
                                                glm::vec3(0.05f, 0.12f, 0.22f),
                                                std::pow(horizon, 1.4f));
                nightColor += glm::vec3(0.28f, 0.33f, 0.42f) *
                              std::pow(glm::max(glm::dot(dir, NIGHT_PRESET.direction), 0.0f), 88.0f);

                glm::vec3 color = glm::clamp(glm::max(dayColor, nightColor), 0.0f, 1.0f);
                const std::size_t idx = static_cast<std::size_t>(y * size + x) * 3;
                pixels[idx + 0] = encode(color.r);
                pixels[idx + 1] = encode(color.g);
                pixels[idx + 2] = encode(color.b);
            }
        }

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB, size, size, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    return tex;
}

static std::array<FrustumPlane, 6> extractFrustumPlanes(const glm::mat4& m)
{
    std::array<FrustumPlane, 6> planes;
    const glm::mat4 t = glm::transpose(m);
    const glm::vec4 rows[4] = {t[0], t[1], t[2], t[3]};

    const glm::vec4 combos[6] = {
        rows[3] + rows[0], rows[3] - rows[0],
        rows[3] + rows[1], rows[3] - rows[1],
        rows[3] + rows[2], rows[3] - rows[2]
    };

    for (int i = 0; i < 6; ++i) {
        const glm::vec3 n(combos[i].x, combos[i].y, combos[i].z);
        const float invLen = 1.0f / glm::max(glm::length(n), 1e-5f);
        planes[i].normal = n * invLen;
        planes[i].d = combos[i].w * invLen;
    }
    return planes;
}

static bool sphereVisible(const std::array<FrustumPlane, 6>& planes, const glm::vec3& center, float radius)
{
    for (const auto& plane : planes) {
        if (glm::dot(plane.normal, center) + plane.d < -radius) {
            return false;
        }
    }
    return true;
}

static void pushHudQuad(std::vector<float>& verts,
                        float x0, float y0, float x1, float y1,
                        const glm::vec3& color)
{
    const float quad[] = {
        x0, y0, color.r, color.g, color.b,
        x1, y0, color.r, color.g, color.b,
        x1, y1, color.r, color.g, color.b,
        x0, y0, color.r, color.g, color.b,
        x1, y1, color.r, color.g, color.b,
        x0, y1, color.r, color.g, color.b
    };
    verts.insert(verts.end(), std::begin(quad), std::end(quad));
}

static const std::array<const char*, 7>* glyphPattern(char c)
{
    static const std::array<const char*, 7> A = {"01110","10001","10001","11111","10001","10001","10001"};
    static const std::array<const char*, 7> B = {"11110","10001","10001","11110","10001","10001","11110"};
    static const std::array<const char*, 7> C = {"01111","10000","10000","10000","10000","10000","01111"};
    static const std::array<const char*, 7> D = {"11110","10001","10001","10001","10001","10001","11110"};
    static const std::array<const char*, 7> E = {"11111","10000","10000","11110","10000","10000","11111"};
    static const std::array<const char*, 7> G = {"01111","10000","10000","10111","10001","10001","01111"};
    static const std::array<const char*, 7> H = {"10001","10001","10001","11111","10001","10001","10001"};
    static const std::array<const char*, 7> I = {"11111","00100","00100","00100","00100","00100","11111"};
    static const std::array<const char*, 7> L = {"10000","10000","10000","10000","10000","10000","11111"};
    static const std::array<const char*, 7> M = {"10001","11011","10101","10001","10001","10001","10001"};
    static const std::array<const char*, 7> N = {"10001","11001","10101","10011","10001","10001","10001"};
    static const std::array<const char*, 7> R = {"11110","10001","10001","11110","10100","10010","10001"};
    static const std::array<const char*, 7> S = {"01111","10000","10000","01110","00001","00001","11110"};
    static const std::array<const char*, 7> T = {"11111","00100","00100","00100","00100","00100","00100"};
    static const std::array<const char*, 7> U = {"10001","10001","10001","10001","10001","10001","01110"};
    static const std::array<const char*, 7> V = {"10001","10001","10001","10001","10001","01010","00100"};
    static const std::array<const char*, 7> Y = {"10001","10001","01010","00100","00100","00100","00100"};
    static const std::array<const char*, 7> K = {"10001","10010","10100","11000","10100","10010","10001"};
    static const std::array<const char*, 7> P = {"11110","10001","10001","11110","10000","10000","10000"};
    static const std::array<const char*, 7> SPACE = {"00000","00000","00000","00000","00000","00000","00000"};

    switch (c) {
        case 'A': return &A; case 'B': return &B; case 'C': return &C; case 'D': return &D;
        case 'E': return &E; case 'G': return &G; case 'H': return &H; case 'I': return &I;
        case 'K': return &K; case 'L': return &L; case 'M': return &M; case 'N': return &N;
        case 'P': return &P; case 'R': return &R; case 'S': return &S; case 'T': return &T;
        case 'U': return &U; case 'V': return &V; case 'Y': return &Y; case ' ': return &SPACE;
        default: return nullptr;
    }
}

static void pushHudText(std::vector<float>& verts,
                        float x,
                        float y,
                        const std::string& text,
                        float scale,
                        const glm::vec3& color)
{
    float cursor = x;
    for (char raw : text) {
        const char c = static_cast<char>(std::toupper(static_cast<unsigned char>(raw)));
        const auto* glyph = glyphPattern(c);
        if (!glyph) {
            cursor += scale * 6.0f;
            continue;
        }
        for (int row = 0; row < 7; ++row) {
            for (int col = 0; col < 5; ++col) {
                if ((*glyph)[row][col] != '1') {
                    continue;
                }
                const float x0 = cursor + static_cast<float>(col) * scale;
                const float y0 = y + static_cast<float>(row) * scale;
                pushHudQuad(verts, x0, y0, x0 + scale, y0 + scale, color);
            }
        }
        cursor += scale * 6.0f;
    }
}

static void drawHudQuads(const std::vector<float>& verts)
{
    if (verts.empty()) {
        return;
    }
    glUseProgram(overlayProgram);
    glBindVertexArray(hudVAO);
    glBindBuffer(GL_ARRAY_BUFFER, hudVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(verts.size() * sizeof(float)), verts.data());
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(verts.size() / 5));
    glBindVertexArray(0);
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
    skyboxProgram = loadShaders("shaders/skybox_vshader.glsl", "shaders/skybox_fshader.glsl");
    overlayProgram = loadShaders("shaders/overlay_vshader.glsl", "shaders/overlay_fshader.glsl");
    groundVAO = createVAO(groundVertices, sizeof(groundVertices));
    skyboxVAO = createPositionOnlyVAO(skyboxVertices, sizeof(skyboxVertices));
    createDynamicHudBuffer();

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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.02f, 0.03f, 0.05f, 1.0f);
    fallbackWhiteTex = createFallbackWhiteTexture();
    skyboxCubemap = createProceduralSkyboxCubemap();

    glUseProgram(shaderProgram);
    setUniform(shaderProgram, "diffuseMap", 0);
    setUniform(shaderProgram, "hasTexture", 0);
    setUniform(shaderProgram, "materialMode", 0);

    glUseProgram(skyboxProgram);
    setUniform(skyboxProgram, "skybox", 0);
}

static void drawModel(const Model& model)
{
    const glm::mat4 view = camera.viewMatrix();
    const glm::mat4 projection = glm::perspective(glm::radians(60.0f),
                                                  static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight),
                                                  0.1f, farPlane);
    const std::array<FrustumPlane, 6> frustum = extractFrustumPlanes(projection * view * sceneRootTransform);

    for (const auto& mesh : model.meshes) {
        if (mesh.boundsRadius > 0.0f &&
            !sphereVisible(frustum, mesh.boundsCenter, mesh.boundsRadius)) {
            ++lastCulledMeshes;
            continue;
        }
        ++lastVisibleMeshes;
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

static void drawSkybox(const glm::mat4& view, const glm::mat4& projection, const LightPreset& lightPreset)
{
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glUseProgram(skyboxProgram);
    const glm::mat4 skyView = glm::mat4(glm::mat3(view));
    setUniform(skyboxProgram, "view", skyView);
    setUniform(skyboxProgram, "projection", projection);
    setUniform(skyboxProgram, "blendFactor", nightMode ? 1.0f : 0.0f);
    setUniform(skyboxProgram, "skyTint", lightPreset.skyTint);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxCubemap);
    glBindVertexArray(skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
}

static void drawHud(const glm::mat4& view, const glm::mat4& projection)
{
    std::vector<float> verts;
    verts.reserve(1024);

    if (showMinimap) {
        const float mapSize = 170.0f;
        const float margin = 18.0f;
        const float x0 = margin;
        const float y0 = static_cast<float>(framebufferHeight) - mapSize - margin;
        const float x1 = x0 + mapSize;
        const float y1 = y0 + mapSize;

        pushHudQuad(verts, x0, y0, x1, y1, nightMode ? glm::vec3(0.07f, 0.10f, 0.14f)
                                                      : glm::vec3(0.90f, 0.93f, 0.88f));
        pushHudQuad(verts, x0 + 4.0f, y0 + 4.0f, x1 - 4.0f, y1 - 4.0f,
                    nightMode ? glm::vec3(0.11f, 0.14f, 0.18f) : glm::vec3(0.80f, 0.82f, 0.76f));

        const glm::vec3 min = campusModel.bboxMin * sceneScale;
        const glm::vec3 max = campusModel.bboxMax * sceneScale;
        const float spanX = glm::max(max.x - min.x, 1.0f);
        const float spanZ = glm::max(max.z - min.z, 1.0f);
        const auto mapPoint = [&](const glm::vec3& p) {
            const float nx = (p.x - min.x) / spanX;
            const float nz = (p.z - min.z) / spanZ;
            return glm::vec2(x0 + 8.0f + nx * (mapSize - 16.0f),
                             y1 - 8.0f - nz * (mapSize - 16.0f));
        };

        for (const auto& landmark : LANDMARKS) {
            const glm::vec2 pt = mapPoint(landmark.position * sceneScale);
            pushHudQuad(verts, pt.x - 3.0f, pt.y - 3.0f, pt.x + 3.0f, pt.y + 3.0f, landmark.color);
            pushHudText(verts, pt.x + 6.0f, pt.y - 6.0f, landmark.shortLabel, 1.5f, landmark.color);
        }

        const glm::vec2 camPt = mapPoint(camera.position);
        pushHudQuad(verts, camPt.x - 4.0f, camPt.y - 4.0f, camPt.x + 4.0f, camPt.y + 4.0f,
                    glm::vec3(0.98f, 0.96f, 0.92f));
        const glm::vec3 camFront = camera.front();
        glm::vec2 flatDir(camFront.x, -camFront.z);
        if (glm::length(flatDir) > 1e-4f) {
            flatDir = glm::normalize(flatDir) * 10.0f;
            pushHudQuad(verts, camPt.x + flatDir.x - 2.0f, camPt.y + flatDir.y - 2.0f,
                        camPt.x + flatDir.x + 2.0f, camPt.y + flatDir.y + 2.0f, glm::vec3(0.98f, 0.72f, 0.28f));
        }
    }

    for (const auto& landmark : LANDMARKS) {
        const glm::vec4 clip = projection * view * glm::vec4(landmark.position * sceneScale, 1.0f);
        if (clip.w <= 0.0f) {
            continue;
        }
        const glm::vec3 ndc = glm::vec3(clip) / clip.w;
        if (ndc.z < -1.0f || ndc.z > 1.0f || std::abs(ndc.x) > 1.05f || std::abs(ndc.y) > 1.05f) {
            continue;
        }

        const float screenX = (ndc.x * 0.5f + 0.5f) * static_cast<float>(framebufferWidth);
        const float screenY = (1.0f - (ndc.y * 0.5f + 0.5f)) * static_cast<float>(framebufferHeight);
        pushHudQuad(verts, screenX - 8.0f, screenY - 22.0f, screenX + 8.0f, screenY - 6.0f, landmark.color);
        pushHudQuad(verts, screenX - 2.0f, screenY - 6.0f, screenX + 2.0f, screenY + 8.0f, landmark.color * 0.85f);
        pushHudText(verts, screenX + 12.0f, screenY - 20.0f, landmark.shortLabel, 2.0f, landmark.color);
    }

    const glm::vec3 statusColor = nightMode ? glm::vec3(0.82f, 0.88f, 0.98f) : glm::vec3(0.16f, 0.21f, 0.28f);
    const std::string modeText = nightMode ? "NIGHT" : "DAY";
    pushHudText(verts, static_cast<float>(framebufferWidth) - 100.0f, 18.0f, modeText, 2.0f, statusColor);
    if (showMinimap) {
        pushHudText(verts, 24.0f, static_cast<float>(framebufferHeight) - 28.0f, "MAP", 1.7f,
                    nightMode ? glm::vec3(0.84f, 0.89f, 0.97f) : glm::vec3(0.18f, 0.20f, 0.18f));
    }

    glDisable(GL_DEPTH_TEST);
    glUseProgram(overlayProgram);
    glUniform2f(glGetUniformLocation(overlayProgram, "viewportSize"),
                static_cast<float>(framebufferWidth), static_cast<float>(framebufferHeight));
    drawHudQuads(verts);
    glEnable(GL_DEPTH_TEST);
}


void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    const glm::mat4 view = camera.viewMatrix();
    const glm::mat4 projection = glm::perspective(glm::radians(60.0f),
                                                  static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight),
                                                  0.1f, farPlane);
    const LightPreset& lightPreset = nightMode ? NIGHT_PRESET : DAY_PRESET;

    drawSkybox(view, projection, lightPreset);

    glUseProgram(shaderProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fallbackWhiteTex);
    setUniform(shaderProgram, "view", view);
    setUniform(shaderProgram, "projection", projection);
    setUniform(shaderProgram, "viewPos", camera.position);
    setUniform(shaderProgram, "lightDir", lightPreset.direction);
    setUniform(shaderProgram, "lightColor", lightPreset.color);
    setUniform(shaderProgram, "ambientTint", lightPreset.ambientTint);
    setUniform(shaderProgram, "nightBlend", nightMode ? 1.0f : 0.0f);

    if (!campusModel.sceneHasTerrain) {
        setUniform(shaderProgram, "model", glm::mat4(1.0f));
        setUniform(shaderProgram, "hasTexture", 0);
        setUniform(shaderProgram, "materialMode", 0);
        setUniform(shaderProgram, "objectColor", glm::vec3(0.30f, 0.55f, 0.25f));
        glBindVertexArray(groundVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    setUniform(shaderProgram, "model", sceneRootTransform);
    lastVisibleMeshes = 0;
    lastCulledMeshes = 0;
    drawModel(campusModel);
    drawModel(odeonModel);
    drawModel(clockTowerModel);
    glBindVertexArray(0);

    drawHud(view, projection);
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------
static bool flyKeyWasPressed = false;
static bool probeKeyWasPressed = false;
static bool nightKeyWasPressed = false;
static bool minimapKeyWasPressed = false;
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

    const bool nightKeyDown = glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS;
    if (nightKeyDown && !nightKeyWasPressed) {
        nightMode = !nightMode;
        std::cout << (nightMode ? "Night mode ON" : "Night mode OFF") << std::endl;
    }
    nightKeyWasPressed = nightKeyDown;

    const bool minimapKeyDown = glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS;
    if (minimapKeyDown && !minimapKeyWasPressed) {
        showMinimap = !showMinimap;
        std::cout << (showMinimap ? "Minimap ON" : "Minimap OFF") << std::endl;
    }
    minimapKeyWasPressed = minimapKeyDown;

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
    std::cout << "Controls: WASD = move, Mouse = look, Shift = sprint, Alt = turbo, F = fly mode, Space/C = up/down (fly), N = day/night, M = minimap, P = print camera probe, 1/2/3 = landmarks, ESC = quit" << std::endl;

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
