#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct Mesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLsizei indexCount = 0;
    GLuint diffuseTex = 0;
    glm::vec3 baseColor = glm::vec3(1.0f);
};

struct TerrainTriangle {
    glm::vec3 a;
    glm::vec3 b;
    glm::vec3 c;
};

struct ObstacleAABB {
    glm::vec2 minXZ;
    glm::vec2 maxXZ;
    float minY = 0.0f;
    float maxY = 0.0f;
};

struct Model {
    std::vector<Mesh> meshes;
    std::vector<TerrainTriangle> walkSurface;
    std::vector<ObstacleAABB> obstacles;
    glm::vec3 bboxMin = glm::vec3(0.0f);
    glm::vec3 bboxMax = glm::vec3(0.0f);
    bool sceneHasTerrain = false;
};

Model loadModel(const std::string& path);
