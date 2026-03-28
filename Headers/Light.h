#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class Light {
public:
    glm::vec3 color;
    float intensity;

    Light(glm::vec3 col, float ind) : color(col), intensity(ind) {}
};

class PointLight : public Light {
public:
    glm::vec3 position;

    PointLight(glm::vec3 pos, glm::vec3 col, float ind)
        : Light(col, ind), position(pos) {
    }
};

class DirectionLight : public Light {
public:
    glm::vec3 direction;

    DirectionLight(glm::vec3 dir, glm::vec3 col, float ind)
        : Light(col, ind), direction(glm::normalize(dir)) {
    }
};