#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    glm::vec3 position;
    float yaw   = -90.0f;   // look along -Z initially
    float pitch  = 0.0f;
    float speed  = 5.0f;    // meters per second
    float sensitivity = 0.1f;
    float walkHeight;        // fixed Y for walking mode

    Camera(glm::vec3 pos = glm::vec3(0.0f, 1.7f, 0.0f))
        : position(pos), walkHeight(pos.y) {}

    glm::vec3 front() const {
        glm::vec3 f;
        f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        f.y = sin(glm::radians(pitch));
        f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        return glm::normalize(f);
    }

    glm::vec3 right() const {
        return glm::normalize(glm::cross(front(), glm::vec3(0.0f, 1.0f, 0.0f)));
    }

    glm::mat4 viewMatrix() const {
        return glm::lookAt(position, position + front(), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    void processKeyboard(int direction, float dt) {
        float velocity = speed * dt;
        glm::vec3 flatFront = glm::normalize(glm::vec3(front().x, 0.0f, front().z));

        switch (direction) {
            case 0: position += flatFront * velocity; break;  // W
            case 1: position -= flatFront * velocity; break;  // S
            case 2: position -= right() * velocity;   break;  // A
            case 3: position += right() * velocity;   break;  // D
        }
        position.y = walkHeight; // stay on ground
    }

    void processMouse(float xoffset, float yoffset) {
        yaw   += xoffset * sensitivity;
        pitch += yoffset * sensitivity;
        if (pitch >  89.0f) pitch =  89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
    }
};
