#pragma once

#include "Headers/Model.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

class Player {
public:
    Model3D model;
    float yaw = -90.0f;
    float moveSpeed = 12.0f;
    float turnSpeed = 95.0f;
    float visualYawOffset = 90.0f;

    glm::vec3 getForward() const {
        return glm::normalize(glm::vec3(
            cos(glm::radians(yaw)),
            0.0f,
            sin(glm::radians(yaw))
        ));
    }

    void syncRotation() {
        model.rotation.y = -yaw + visualYawOffset;
    }

    void moveForward(float deltaTime) {
        model.position += getForward() * moveSpeed * deltaTime;
    }

    void moveBackward(float deltaTime) {
        model.position -= getForward() * moveSpeed * deltaTime;
    }

    void turnLeft(float deltaTime) {
        yaw -= turnSpeed * deltaTime;
        syncRotation();
    }

    void turnRight(float deltaTime) {
        yaw += turnSpeed * deltaTime;
        syncRotation();
    }

    glm::vec3 getCameraTarget() const {
        return model.position + glm::vec3(0.0f, 2.5f, 0.0f);
    }

    glm::vec3 getFrontLightPos() const {
        return model.position + glm::vec3(0.0f, 1.8f, 0.0f) + getForward() * 5.0f;
    }
    
    glm::vec3 getBinocularPosition() const {
        return model.position + glm::vec3(0.0f, 3.8f, 0.0f) + getForward() * 5.0f;
    }

    void draw(GLuint shaderProgram, GLuint VAO, int indexCount) {
        model.draw(shaderProgram, VAO, indexCount);
    }
};