#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Base camera class
class Camera {
protected:
    glm::mat4 projectionMatrix = glm::mat4(1.0f);

public:
    virtual glm::mat4 getViewMatrix() = 0;
    virtual glm::mat4 getProjectionMatrix() = 0;
};


class ThirdPersonCamera : public Camera {
private:
    float radius;
    float yaw;
    float pitch;
    glm::vec3 target;

public:
    ThirdPersonCamera(float r, glm::vec3 startTarget)
        : radius(r), yaw(-90.0f), pitch(-15.0f), target(startTarget) {
        projectionMatrix = glm::perspective(glm::radians(60.0f), 800.0f / 600.0f, 0.1f, 250.0f);
    }

    void setTarget(glm::vec3 newTarget) {
        target = newTarget;
    }

    void updateAngles(float xoffset, float yoffset) {
        yaw += xoffset * 0.2f;
        pitch += yoffset * 0.2f;

        if (pitch > 20.0f) pitch = 20.0f;
        if (pitch < -15.0f) pitch = -15.0f;
    }

    glm::vec3 getPosition() const {
        glm::vec3 pos;
        pos.x = target.x + radius * cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        pos.y = target.y + radius * sin(glm::radians(pitch)) + 6.0f;
        pos.z = target.z + radius * sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        return pos;
    }

    glm::mat4 getViewMatrix() override {
        return glm::lookAt(getPosition(), target, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::mat4 getProjectionMatrix() override {
        return projectionMatrix;
    }
};

class BinocularCamera : public Camera {
private:
    glm::vec3 position;
    float yaw;
    float pitch;
    float fov;

public:
    BinocularCamera(glm::vec3 startPos = glm::vec3(0.0f))
        : position(startPos), yaw(-90.0f), pitch(0.0f), fov(25.0f) {
    }

    void setPosition(glm::vec3 newPos) {
        position = newPos;
    }

    void setAngles(float newYaw, float newPitch) {
        yaw = newYaw;
        pitch = newPitch;

        if (pitch > 60.0f) pitch = 60.0f;
        if (pitch < -60.0f) pitch = -60.0f;
    }

    void rotate(float yawDelta, float pitchDelta) {
        yaw += yawDelta;
        pitch += pitchDelta;

        if (pitch > 60.0f) pitch = 60.0f;
        if (pitch < -60.0f) pitch = -60.0f;
    }

    void zoom(float delta) {
        fov += delta;

        if (fov < 10.0f) fov = 10.0f;
        if (fov > 45.0f) fov = 45.0f;
    }

    glm::vec3 getForward() const {
        glm::vec3 dir;
        dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        dir.y = sin(glm::radians(pitch));
        dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        return glm::normalize(dir);
    }

    glm::vec3 getPosition() const {
        return position;
    }

    glm::mat4 getViewMatrix() override {
        return glm::lookAt(position, position + getForward(), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::mat4 getProjectionMatrix() override {
        return glm::perspective(glm::radians(fov), 800.0f / 600.0f, 0.5f, 1000.0f);
    }
};

class OrthoCamera : public Camera {
private:
    glm::vec3 centerTarget;

public:
    OrthoCamera(glm::vec3 startTarget = glm::vec3(0.0f))
        : centerTarget(startTarget) {
        projectionMatrix = glm::ortho(-60.0f, 60.0f, -60.0f, 60.0f, 0.1f, 1000.0f);
    }

    void setTarget(glm::vec3 newTarget) {
        centerTarget = newTarget;
    }

    void pan(float dx, float dz) {
        centerTarget.x += dx;
        centerTarget.z += dz;
    }

    glm::vec3 getPosition() const {
        return glm::vec3(centerTarget.x, 80.0f, centerTarget.z);
    }

    glm::mat4 getViewMatrix() override {
        return glm::lookAt(getPosition(), centerTarget, glm::vec3(0.0f, 0.0f, -1.0f));
    }

    glm::mat4 getProjectionMatrix() override {
        return projectionMatrix;
    }
};