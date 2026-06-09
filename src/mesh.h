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
    glm::vec3 bboxMin = glm::vec3(0.0f);
    glm::vec3 bboxMax = glm::vec3(0.0f);
    glm::vec3 boundsCenter = glm::vec3(0.0f);
    float boundsRadius = 0.0f;
    int materialMode = 0;  // 0 = plain, 1 = facade, 2 = roof, 3 = road/path, 4 = rectorate facade, 5 = grass, 6 = stone, 11 = water, 12 = stair stone, 13 = fence, 14 = foliage, 15 = trunk
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
    std::vector<glm::vec3> roadPoints;
    std::vector<glm::vec3> perimeterPoints;
    glm::vec3 bboxMin = glm::vec3(0.0f);
    glm::vec3 bboxMax = glm::vec3(0.0f);
    bool sceneHasTerrain = false;
};

Model loadModel(const std::string& path);
