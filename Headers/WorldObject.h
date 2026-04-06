#pragma once

#include "Model.h"
#include "Shader.h"

#include <glad/glad.h>
#include <vector>

// Represents a renderable world object, including its geometry and textures
struct WorldObjectAsset {
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    size_t indexCount = 0;

    GLuint diffuseTexture = 0;
    GLuint normalTexture = 0;
};

struct WorldObjectInstance {
    Model3D model;
    bool useNormalMap = true;
};

inline void drawWorldObject(const Shader& shader, const WorldObjectAsset& asset, const WorldObjectInstance& instance) {
    shader.setBool("useNormalMap", instance.useNormalMap);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, asset.diffuseTexture);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, asset.normalTexture);

    instance.model.draw(shader.ID, asset.VAO, (int)asset.indexCount);
}