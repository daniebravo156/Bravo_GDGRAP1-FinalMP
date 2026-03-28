#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <fstream>

// Camera Classes
class MyCamera {
protected:
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    glm::mat4 projectionMatrix = glm::mat4(1.0f);
public:
    virtual glm::mat4 getViewMatrix() = 0;
    virtual glm::mat4 getProjectionMatrix() = 0;
};

class PerspectiveCamera : public MyCamera {
private:
    float radius, yaw, pitch;
    glm::vec3 centerTarget;
public:
    PerspectiveCamera(float r, glm::vec3 target)
        : radius(r), yaw(-90.0f), pitch(-15.0f), centerTarget(target) {
        projectionMatrix = glm::perspective(glm::radians(60.0f), 800.0f / 600.0f, 0.1f, 1000.0f);
    }

    void setTarget(glm::vec3 target) {
        centerTarget = target;
    }

    void updateAngles(float xoffset, float yoffset) {
        yaw += xoffset * 0.2f;
        pitch += yoffset * 0.2f;
        if (pitch > 45.0f) pitch = 45.0f;
        if (pitch < -45.0f) pitch = -45.0f;
    }

    glm::vec3 getPosition() {
        glm::vec3 pos;
        pos.x = centerTarget.x + radius * cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        pos.y = centerTarget.y + radius * sin(glm::radians(pitch)) + 6.0f;
        pos.z = centerTarget.z + radius * sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        return pos;
    }

    glm::mat4 getViewMatrix() override {
        return glm::lookAt(getPosition(), centerTarget, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::mat4 getProjectionMatrix() override {
        return projectionMatrix;
    }
};

class OrthoCamera : public MyCamera {
private:
    glm::vec3 centerTarget;
public:
    OrthoCamera(glm::vec3 target = glm::vec3(0.0f)) : centerTarget(target) {
        projectionMatrix = glm::ortho(-60.0f, 60.0f, -60.0f, 60.0f, 0.1f, 300.0f);
    }

    void setTarget(glm::vec3 target) {
        centerTarget = target;
    }

    glm::mat4 getViewMatrix() override {
        return glm::lookAt(
            glm::vec3(centerTarget.x, 80.0f, centerTarget.z),
            centerTarget,
            glm::vec3(0.0f, 0.0f, -1.0f)
        );
    }

    glm::mat4 getProjectionMatrix() override {
        return projectionMatrix;
    }
};

// Light Classes
class Light {
public:
    glm::vec3 color;
    float intensity;
    Light(glm::vec3 col, float ind) : color(col), intensity(ind) {}
};

class PointLight : public Light {
public:
    glm::vec3 position;
    PointLight(glm::vec3 pos, glm::vec3 col, float ind) : Light(col, ind), position(pos) {}
};

class DirectionLight : public Light {
public:
    glm::vec3 direction;
    DirectionLight(glm::vec3 dir, glm::vec3 col, float ind) : Light(col, ind), direction(dir) {}
};

// Model Struct 
struct Model3D {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    void draw(GLuint shaderProgram, GLuint VAO, int indexCount) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scale);

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};

//  Globals 
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

int activeCamera = 1;

PerspectiveCamera perspCam(18.0f, glm::vec3(0.0f, 2.0f, 0.0f));
OrthoCamera orthoCam(glm::vec3(0.0f));

bool firstMouse = true;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;

glm::vec3 tankPosition(0.0f, 0.0f, 0.0f);

PointLight pLight(glm::vec3(0.0f, 3.0f, 8.0f), glm::vec3(1.0f, 0.95f, 0.85f), 1.0f);
DirectionLight dLight(glm::normalize(glm::vec3(-0.3f, -1.0f, -0.2f)), glm::vec3(0.45f, 0.5f, 0.7f), 0.7f);

//  Helper Functions 
std::string get_file_contents(const char* filename) {
    std::string contents = "";
    std::ifstream in(filename, std::ios::binary);
    if (in) {
        in.seekg(0, std::ios::end);
        contents.resize((size_t)in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&contents[0], contents.size());
        in.close();
    }
    else {
        std::cout << "FAILED TO OPEN FILE: " << filename << std::endl;
    }
    return contents;
}

GLuint loadTexture(const char* path) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);

    if (data) {
        GLenum format = GL_RGB;
        if (nrChannels == 4) format = GL_RGBA;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        std::cout << "FAILED TO LOAD TEXTURE " << path << std::endl;
    }

    stbi_image_free(data);
    return textureID;
}

bool loadMesh(const char* path, GLuint& VAO, GLuint& VBO, GLuint& EBO, size_t& indexCount) {
    bool isSuccessful = true;
    tinyobj::attrib_t attributes;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attributes, &shapes, &materials, &warn, &err, path)) {
        std::cout << "ERROR LOADING " << path << ": " << err << std::endl;
        isSuccessful = false;
    }
    else {
        std::vector<GLuint> mesh_indices;
        std::vector<GLfloat> mesh_vertices;

        for (size_t i = 0; i < shapes[0].mesh.indices.size(); i++) {
            tinyobj::index_t vData = shapes[0].mesh.indices[i];
            mesh_indices.push_back((GLuint)i);

            mesh_vertices.push_back(attributes.vertices[3 * vData.vertex_index + 0]);
            mesh_vertices.push_back(attributes.vertices[3 * vData.vertex_index + 1]);
            mesh_vertices.push_back(attributes.vertices[3 * vData.vertex_index + 2]);

            if (attributes.normals.size() > 0) {
                mesh_vertices.push_back(attributes.normals[3 * vData.normal_index + 0]);
                mesh_vertices.push_back(attributes.normals[3 * vData.normal_index + 1]);
                mesh_vertices.push_back(attributes.normals[3 * vData.normal_index + 2]);
            }
            else {
                mesh_vertices.push_back(0.0f);
                mesh_vertices.push_back(1.0f);
                mesh_vertices.push_back(0.0f);
            }

            if (attributes.texcoords.size() > 0) {
                mesh_vertices.push_back(attributes.texcoords[2 * vData.texcoord_index + 0]);
                mesh_vertices.push_back(attributes.texcoords[2 * vData.texcoord_index + 1]);
            }
            else {
                mesh_vertices.push_back(0.0f);
                mesh_vertices.push_back(0.0f);
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

        GLsizei stride = 8 * sizeof(float);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        indexCount = mesh_indices.size();
    }

    return isSuccessful;
}

void createGroundPlane(GLuint& VAO, GLuint& VBO, GLuint& EBO, size_t& indexCount) {
    float s = 120.0f;
    float y = 0.0f;

    GLfloat vertices[] = {
        -s, y, -s,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
         s, y, -s,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
         s, y,  s,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
        -s, y,  s,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f
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

    GLsizei stride = 8 * sizeof(float);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    indexCount = 6;
}

GLuint loadCubemap(std::vector<std::string> faces) {
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

//  Input 
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if ((action == GLFW_PRESS || action == GLFW_REPEAT)) {
        if (key == GLFW_KEY_1) activeCamera = 1;
        if (key == GLFW_KEY_2) activeCamera = 2;
    }
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    if (activeCamera == 1) {
        float xpos = static_cast<float>(xposIn);
        float ypos = static_cast<float>(yposIn);

        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos;
        lastX = xpos;
        lastY = ypos;

        perspCam.updateAngles(xoffset, yoffset);
    }
}

//  Main 
int main() {
    int appStatus = 0;

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "GDGRAP1 Final - Elden's Hill", NULL, NULL);

    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGL()) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glEnable(GL_DEPTH_TEST);

    //  Compile object shaders 
    std::string objectVertCode = get_file_contents("Shaders/object.vert");
    std::string objectFragCode = get_file_contents("Shaders/object.frag");
    const char* objectV = objectVertCode.c_str();
    const char* objectF = objectFragCode.c_str();

    GLuint objectVertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(objectVertexShader, 1, &objectV, NULL);
    glCompileShader(objectVertexShader);

    GLuint objectFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(objectFragmentShader, 1, &objectF, NULL);
    glCompileShader(objectFragmentShader);

    GLuint objectShaderProgram = glCreateProgram();
    glAttachShader(objectShaderProgram, objectVertexShader);
    glAttachShader(objectShaderProgram, objectFragmentShader);
    glLinkProgram(objectShaderProgram);
    glDeleteShader(objectVertexShader);
    glDeleteShader(objectFragmentShader);

    //  Compile skybox shaders 
    std::string skyboxVertCode = get_file_contents("Shaders/skybox.vert");
    std::string skyboxFragCode = get_file_contents("Shaders/skybox.frag");
    const char* skyboxV = skyboxVertCode.c_str();
    const char* skyboxF = skyboxFragCode.c_str();

    GLuint skyboxVertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(skyboxVertexShader, 1, &skyboxV, NULL);
    glCompileShader(skyboxVertexShader);

    GLuint skyboxFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(skyboxFragmentShader, 1, &skyboxF, NULL);
    glCompileShader(skyboxFragmentShader);

    GLuint skyboxShaderProgram = glCreateProgram();
    glAttachShader(skyboxShaderProgram, skyboxVertexShader);
    glAttachShader(skyboxShaderProgram, skyboxFragmentShader);
    glLinkProgram(skyboxShaderProgram);
    glDeleteShader(skyboxVertexShader);
    glDeleteShader(skyboxFragmentShader);

    //  Load textures 
    GLuint groundTexture = loadTexture("3D/field.jpg");
    GLuint tankTexture = loadTexture("3D/tank_diffuse.PNG");

    glUseProgram(objectShaderProgram);
    glUniform1i(glGetUniformLocation(objectShaderProgram, "tex0"), 0);

    glUseProgram(skyboxShaderProgram);
    glUniform1i(glGetUniformLocation(skyboxShaderProgram, "skybox"), 0);

    //  Load tank mesh 
    GLuint tankVAO, tankVBO, tankEBO;
    size_t tankIndices;

    if (!loadMesh("3D/tank.obj", tankVAO, tankVBO, tankEBO, tankIndices)) {
        std::cout << "Tank failed to load." << std::endl;
        return -1;
    }

    Model3D tankModel;
    tankModel.position = glm::vec3(0.0f, 0.0f, 0.0f);
    tankModel.scale = glm::vec3(1.0f);

    //  Create ground plane 
    GLuint groundVAO, groundVBO, groundEBO;
    size_t groundIndices;
    createGroundPlane(groundVAO, groundVBO, groundEBO, groundIndices);

    Model3D groundModel;
    groundModel.position = glm::vec3(0.0f);

    //  Skybox geometry 
    float skyboxVertices[] = {
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f
    };

    unsigned int skyboxIndices[] = {
        0, 1, 2, 2, 3, 0,
        1, 5, 6, 6, 2, 1,
        5, 4, 7, 7, 6, 5,
        4, 0, 3, 3, 7, 4,
        3, 2, 6, 6, 7, 3,
        4, 5, 1, 1, 0, 4
    };

    GLuint skyboxVAO, skyboxVBO, skyboxEBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glGenBuffers(1, &skyboxEBO);

    glBindVertexArray(skyboxVAO);

    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyboxEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyboxIndices), skyboxIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    std::vector<std::string> skyboxFaces = {
        "Skybox/skyboxRT.png",
        "Skybox/skyboxLF.png",
        "Skybox/skyboxUP.png",
        "Skybox/skyboxDN.png",
        "Skybox/skyboxFT.png",
        "Skybox/skyboxBK.png"
    };

    GLuint cubemapTexture = loadCubemap(skyboxFaces);

    //  Render loop 
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.03f, 0.03f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        perspCam.setTarget(glm::vec3(0.0f, 2.0f, 0.0f));
        orthoCam.setTarget(glm::vec3(0.0f, 0.0f, 0.0f));

        glm::mat4 view;
        glm::mat4 projection;

        if (activeCamera == 1) {
            view = perspCam.getViewMatrix();
            projection = perspCam.getProjectionMatrix();
        }
        else {
            view = orthoCam.getViewMatrix();
            projection = orthoCam.getProjectionMatrix();
        }

        //  Draw skybox 
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);

        glUseProgram(skyboxShaderProgram);
        glm::mat4 skyView = glm::mat4(glm::mat3(view));

        glUniformMatrix4fv(glGetUniformLocation(skyboxShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(skyView));
        glUniformMatrix4fv(glGetUniformLocation(skyboxShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

        glBindVertexArray(skyboxVAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);

        //  Draw lit objects 
        glUseProgram(objectShaderProgram);

        glUniformMatrix4fv(glGetUniformLocation(objectShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(objectShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glUniform3fv(glGetUniformLocation(objectShaderProgram, "pointLightPos"), 1, glm::value_ptr(pLight.position));
        glUniform3fv(glGetUniformLocation(objectShaderProgram, "pointLightColor"), 1, glm::value_ptr(pLight.color));
        glUniform1f(glGetUniformLocation(objectShaderProgram, "pointLightIntensity"), pLight.intensity);

        glUniform3fv(glGetUniformLocation(objectShaderProgram, "dirLightDirection"), 1, glm::value_ptr(dLight.direction));
        glUniform3fv(glGetUniformLocation(objectShaderProgram, "dirLightColor"), 1, glm::value_ptr(dLight.color));
        glUniform1f(glGetUniformLocation(objectShaderProgram, "dirLightIntensity"), dLight.intensity);

        // Ground
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, groundTexture);
        groundModel.draw(objectShaderProgram, groundVAO, (int)groundIndices);

        // Tank
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tankTexture);
        tankModel.draw(objectShaderProgram, tankVAO, (int)tankIndices);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return appStatus;
}