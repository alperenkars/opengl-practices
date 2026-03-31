#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <string>

#include "shader.h"
#include "camera.h"

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
// Per vertex: position (3) + normal (3) = 6 floats.

// clang-format off
float cubeVertices[] = {
    // positions          // normals
    // Front face (+Z)
    -0.5f, 0.0f,  0.5f,   0, 0, 1,
     0.5f, 0.0f,  0.5f,   0, 0, 1,
     0.5f, 1.0f,  0.5f,   0, 0, 1,
     0.5f, 1.0f,  0.5f,   0, 0, 1,
    -0.5f, 1.0f,  0.5f,   0, 0, 1,
    -0.5f, 0.0f,  0.5f,   0, 0, 1,
    // Back face (-Z)
     0.5f, 0.0f, -0.5f,   0, 0,-1,
    -0.5f, 0.0f, -0.5f,   0, 0,-1,
    -0.5f, 1.0f, -0.5f,   0, 0,-1,
    -0.5f, 1.0f, -0.5f,   0, 0,-1,
     0.5f, 1.0f, -0.5f,   0, 0,-1,
     0.5f, 0.0f, -0.5f,   0, 0,-1,
    // Left face (-X)
    -0.5f, 0.0f, -0.5f,  -1, 0, 0,
    -0.5f, 0.0f,  0.5f,  -1, 0, 0,
    -0.5f, 1.0f,  0.5f,  -1, 0, 0,
    -0.5f, 1.0f,  0.5f,  -1, 0, 0,
    -0.5f, 1.0f, -0.5f,  -1, 0, 0,
    -0.5f, 0.0f, -0.5f,  -1, 0, 0,
    // Right face (+X)
     0.5f, 0.0f,  0.5f,   1, 0, 0,
     0.5f, 0.0f, -0.5f,   1, 0, 0,
     0.5f, 1.0f, -0.5f,   1, 0, 0,
     0.5f, 1.0f, -0.5f,   1, 0, 0,
     0.5f, 1.0f,  0.5f,   1, 0, 0,
     0.5f, 0.0f,  0.5f,   1, 0, 0,
    // Top face (+Y)
    -0.5f, 1.0f,  0.5f,   0, 1, 0,
     0.5f, 1.0f,  0.5f,   0, 1, 0,
     0.5f, 1.0f, -0.5f,   0, 1, 0,
     0.5f, 1.0f, -0.5f,   0, 1, 0,
    -0.5f, 1.0f, -0.5f,   0, 1, 0,
    -0.5f, 1.0f,  0.5f,   0, 1, 0,
    // Bottom face (-Y)
    -0.5f, 0.0f, -0.5f,   0,-1, 0,
     0.5f, 0.0f, -0.5f,   0,-1, 0,
     0.5f, 0.0f,  0.5f,   0,-1, 0,
     0.5f, 0.0f,  0.5f,   0,-1, 0,
    -0.5f, 0.0f,  0.5f,   0,-1, 0,
    -0.5f, 0.0f, -0.5f,   0,-1, 0,
};
// clang-format on

// Ground plane: large quad at y = 0
float groundVertices[] = {
    // positions              // normals
    -500.0f, 0.0f,  500.0f,   0, 1, 0,
     500.0f, 0.0f,  500.0f,   0, 1, 0,
     500.0f, 0.0f, -500.0f,   0, 1, 0,
     500.0f, 0.0f, -500.0f,   0, 1, 0,
    -500.0f, 0.0f, -500.0f,   0, 1, 0,
    -500.0f, 0.0f,  500.0f,   0, 1, 0,
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    // normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
    return vao;
}

static void setUniform(GLuint prog, const char* name, const glm::mat4& m) {
    glUniformMatrix4fv(glGetUniformLocation(prog, name), 1, GL_FALSE, glm::value_ptr(m));
}

static void setUniform(GLuint prog, const char* name, const glm::vec3& v) {
    glUniform3fv(glGetUniformLocation(prog, name), 1, glm::value_ptr(v));
}

// ---------------------------------------------------------------------------
// Init & Display
// ---------------------------------------------------------------------------

void init()
{
    shaderProgram = loadShaders("shaders/vshader.glsl", "shaders/fshader.glsl");
    cubeVAO   = createVAO(cubeVertices,   sizeof(cubeVertices));
    groundVAO = createVAO(groundVertices, sizeof(groundVertices));

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);  // sky blue
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);

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

        setUniform(shaderProgram, "model", model);
        setUniform(shaderProgram, "objectColor", b.color);
        glDrawArrays(GL_TRIANGLES, 0, 36);
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
