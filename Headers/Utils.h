#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <tiny_obj_loader.h>
#include <stb_image.h>

#include <vector>
#include <string>
#include <iostream>
#include <cmath>

// Utility functions for loading textures, cubemaps, and meshes
inline GLuint loadTexture(const char* path, bool flip = true) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(flip);
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);

    if (data) {
        GLenum format = GL_RGB;
        if (nrChannels == 4) format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        std::cout << "FAILED TO LOAD TEXTURE: " << path << std::endl;
    }

    stbi_image_free(data);
    return textureID;
}

inline GLuint loadCubemap(const std::vector<std::string>& faces) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    stbi_set_flip_vertically_on_load(false);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);

        if (data) {
            GLenum format = GL_RGB;
            if (nrChannels == 4) format = GL_RGBA;

            glTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0,
                format,
                width,
                height,
                0,
                format,
                GL_UNSIGNED_BYTE,
                data
            );
        }
        else {
            std::cout << "FAILED TO LOAD CUBEMAP FACE: " << faces[i] << std::endl;
        }

        stbi_image_free(data);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

inline bool loadMeshNormalMapped(const char* path, GLuint& VAO, GLuint& VBO, GLuint& EBO, size_t& indexCount) {
    tinyobj::attrib_t attributes;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attributes, &shapes, &materials, &warn, &err, path, nullptr, true)) {
        std::cout << "ERROR LOADING " << path << ": " << err << std::endl;
        return false;
    }

    std::vector<GLuint> mesh_indices;
    std::vector<GLfloat> mesh_vertices;
    GLuint runningIndex = 0;

    auto getPos = [&](tinyobj::index_t idx) -> glm::vec3 {
        return glm::vec3(
            attributes.vertices[3 * idx.vertex_index + 0],
            attributes.vertices[3 * idx.vertex_index + 1],
            attributes.vertices[3 * idx.vertex_index + 2]
        );
        };

    auto getNormal = [&](tinyobj::index_t idx) -> glm::vec3 {
        if (idx.normal_index >= 0 && !attributes.normals.empty()) {
            return glm::vec3(
                attributes.normals[3 * idx.normal_index + 0],
                attributes.normals[3 * idx.normal_index + 1],
                attributes.normals[3 * idx.normal_index + 2]
            );
        }

        return glm::vec3(0.0f, 1.0f, 0.0f);
        };

    auto getUV = [&](tinyobj::index_t idx) -> glm::vec2 {
        if (idx.texcoord_index >= 0 && !attributes.texcoords.empty()) {
            return glm::vec2(
                attributes.texcoords[2 * idx.texcoord_index + 0],
                attributes.texcoords[2 * idx.texcoord_index + 1]
            );
        }

        return glm::vec2(0.0f, 0.0f);
        };

    auto pushVertex = [&](glm::vec3 pos, glm::vec3 normal, glm::vec2 uv, glm::vec3 tangent, glm::vec3 bitangent) {
        mesh_vertices.push_back(pos.x);
        mesh_vertices.push_back(pos.y);
        mesh_vertices.push_back(pos.z);

        mesh_vertices.push_back(normal.x);
        mesh_vertices.push_back(normal.y);
        mesh_vertices.push_back(normal.z);

        mesh_vertices.push_back(uv.x);
        mesh_vertices.push_back(uv.y);

        mesh_vertices.push_back(tangent.x);
        mesh_vertices.push_back(tangent.y);
        mesh_vertices.push_back(tangent.z);

        mesh_vertices.push_back(bitangent.x);
        mesh_vertices.push_back(bitangent.y);
        mesh_vertices.push_back(bitangent.z);

        mesh_indices.push_back(runningIndex++);
        };

    for (size_t s = 0; s < shapes.size(); s++) {
        std::vector<tinyobj::index_t>& indices = shapes[s].mesh.indices;

        for (size_t i = 0; i + 2 < indices.size(); i += 3) {
            tinyobj::index_t i0 = indices[i + 0];
            tinyobj::index_t i1 = indices[i + 1];
            tinyobj::index_t i2 = indices[i + 2];

            glm::vec3 p0 = getPos(i0);
            glm::vec3 p1 = getPos(i1);
            glm::vec3 p2 = getPos(i2);

            glm::vec3 n0 = getNormal(i0);
            glm::vec3 n1 = getNormal(i1);
            glm::vec3 n2 = getNormal(i2);

            glm::vec2 uv0 = getUV(i0);
            glm::vec2 uv1 = getUV(i1);
            glm::vec2 uv2 = getUV(i2);

            glm::vec3 deltaPos1 = p1 - p0;
            glm::vec3 deltaPos2 = p2 - p0;
            glm::vec2 deltaUV1 = uv1 - uv0;
            glm::vec2 deltaUV2 = uv2 - uv0;

            glm::vec3 tangent(1.0f, 0.0f, 0.0f);
            glm::vec3 bitangent(0.0f, 0.0f, 1.0f);

            float det = deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x;
            if (fabs(det) > 0.00001f) {
                float invDet = 1.0f / det;
                tangent = glm::normalize((deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * invDet);
                bitangent = glm::normalize((deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * invDet);
            }

            pushVertex(p0, n0, uv0, tangent, bitangent);
            pushVertex(p1, n1, uv1, tangent, bitangent);
            pushVertex(p2, n2, uv2, tangent, bitangent);
        }
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mesh_vertices.size(), mesh_vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * mesh_indices.size(), mesh_indices.data(), GL_STATIC_DRAW);

    GLsizei stride = 14 * sizeof(float);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);

    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, (void*)(11 * sizeof(float)));
    glEnableVertexAttribArray(4);

    indexCount = mesh_indices.size();
    return true;
}

inline void createGroundPlane(GLuint& VAO, GLuint& VBO, GLuint& EBO, size_t& indexCount) {
    float s = 120.0f;
    float y = 0.0f;

    GLfloat vertices[] = {
        -s, y, -s,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
         s, y, -s,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
         s, y,  s,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
        -s, y,  s,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,   1.0f, 0.0f, 0.0f,   0.0f, 0.0f, 1.0f
    };

    GLuint indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    GLsizei stride = 14 * sizeof(float);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);

    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, (void*)(11 * sizeof(float)));
    glEnableVertexAttribArray(4);

    indexCount = 6;
}