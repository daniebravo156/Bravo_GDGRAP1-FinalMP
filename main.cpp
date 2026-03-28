#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// file reading helper
std::string get_file_contents(const char* filename)
{
    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        std::cout << "FAILED TO OPEN FILE: " << filename << std::endl;
        return "";
    }

    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize((size_t)in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();
    return contents;
}

// shader compilation helper
GLuint createShaderProgram(const char* vertPath, const char* fragPath)
{
    std::string vertexCode = get_file_contents(vertPath);
    std::string fragmentCode = get_file_contents(fragPath);

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vShaderCode, NULL);
    glCompileShader(vertexShader);

    GLint success;
    GLchar infoLog[1024];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 1024, NULL, infoLog);
        std::cout << "VERTEX SHADER ERROR:\n" << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fShaderCode, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 1024, NULL, infoLog);
        std::cout << "FRAGMENT SHADER ERROR:\n" << infoLog << std::endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 1024, NULL, infoLog);
        std::cout << "SHADER LINK ERROR:\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// texture loading helper
GLuint loadTexture2D(const char* path, bool flip = true)
{
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
        if (nrChannels == 1) format = GL_RED;
        else if (nrChannels == 3) format = GL_RGB;
        else if (nrChannels == 4) format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        std::cout << "FAILED TO LOAD TEXTURE: " << path << std::endl;
    }

    stbi_image_free(data);
    return textureID;
}

// mesh loading helper (w tangent and bitangent calculation for normal mapping)
bool loadPlaneMeshWithTangent(const char* path, GLuint& VAO, GLuint& VBO, GLuint& EBO, size_t& indexCount)
{
    tinyobj::attrib_t attributes;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attributes, &shapes, &materials, &warn, &err, path)) {
        std::cout << "ERROR LOADING " << path << ": " << err << std::endl;
        return false;
    }

    std::vector<GLuint> mesh_indices;
    std::vector<GLfloat> mesh_vertices;

    const std::vector<tinyobj::index_t>& indices = shapes[0].mesh.indices;

	// Loop through faces (triangles) and extract vertex data
    for (size_t i = 0; i < indices.size(); i += 3) {
        tinyobj::index_t ids[3] = { indices[i], indices[i + 1], indices[i + 2] };

        glm::vec3 pos[3];
        glm::vec3 nor[3];
        glm::vec2 uv[3];

        for (int v = 0; v < 3; v++) {
            pos[v] = glm::vec3(
                attributes.vertices[3 * ids[v].vertex_index + 0],
                attributes.vertices[3 * ids[v].vertex_index + 1],
                attributes.vertices[3 * ids[v].vertex_index + 2]
            );

            if (!attributes.normals.empty()) {
                nor[v] = glm::vec3(
                    attributes.normals[3 * ids[v].normal_index + 0],
                    attributes.normals[3 * ids[v].normal_index + 1],
                    attributes.normals[3 * ids[v].normal_index + 2]
                );
            }
            else {
                nor[v] = glm::vec3(0.0f, 1.0f, 0.0f);
            }

            if (!attributes.texcoords.empty()) {
                uv[v] = glm::vec2(
                    attributes.texcoords[2 * ids[v].texcoord_index + 0],
                    attributes.texcoords[2 * ids[v].texcoord_index + 1]
                );
            }
            else {
                uv[v] = glm::vec2(0.0f, 0.0f);
            }
        }

        glm::vec3 deltaPos1 = pos[1] - pos[0];
        glm::vec3 deltaPos2 = pos[2] - pos[0];
        glm::vec2 deltaUV1 = uv[1] - uv[0];
        glm::vec2 deltaUV2 = uv[2] - uv[0];

        float denom = deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x;
        float r = (std::fabs(denom) < 0.00001f) ? 1.0f : (1.0f / denom);

		// Calculate tangent and bitangent vectors
        glm::vec3 tangent = glm::normalize((deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r);
        glm::vec3 bitangent = glm::normalize((deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r);

		// Append vertex data to mesh arrays
        for (int v = 0; v < 3; v++) 
        {
            mesh_vertices.push_back(pos[v].x);
            mesh_vertices.push_back(pos[v].y);
            mesh_vertices.push_back(pos[v].z);

            mesh_vertices.push_back(nor[v].x);
            mesh_vertices.push_back(nor[v].y);
            mesh_vertices.push_back(nor[v].z);

            mesh_vertices.push_back(uv[v].x);
            mesh_vertices.push_back(uv[v].y);

            mesh_vertices.push_back(tangent.x);
            mesh_vertices.push_back(tangent.y);
            mesh_vertices.push_back(tangent.z);

            mesh_vertices.push_back(bitangent.x);
            mesh_vertices.push_back(bitangent.y);
            mesh_vertices.push_back(bitangent.z);

            mesh_indices.push_back((GLuint)mesh_indices.size());
        }
    }

    // Create OpenGL buffers and upload mesh data
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

    glBindVertexArray(0);

    indexCount = mesh_indices.size();
    return true;
}

// cubemap loading helper
GLuint loadCubemap(const std::vector<std::string>& faces)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	stbi_set_flip_vertically_on_load(false); // Cubemap textures should not be flipped

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        }
        else {
            std::cout << "FAILED TO LOAD CUBEMAP FACE: " << faces[i] << std::endl;
        }
        stbi_image_free(data);
    }
	// Set cubemap texture parameters
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

// Represent of 3D model's transform
struct Model3D
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    glm::mat4 getMatrix() const
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scale);
        return model;
    }
};

// Skybox cube vertices and indices
float skyboxVertices[] = {
    -1.f, -1.f,  1.f,
     1.f, -1.f,  1.f,
     1.f, -1.f, -1.f,
    -1.f, -1.f, -1.f,
    -1.f,  1.f,  1.f,
     1.f,  1.f,  1.f,
     1.f,  1.f, -1.f,
    -1.f,  1.f, -1.f
};
unsigned int skyboxIndices[] = {
    1,2,6, 6,5,1,
    0,4,7, 7,3,0,
    4,5,6, 6,7,4,
    0,3,2, 2,1,0,
    0,1,5, 5,4,0,
    3,7,6, 6,2,3
};

// main
int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    //create window and checkers
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Quiz02 - Danielle R. Bravo", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGL()) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Load shaders, textures, and meshes
    GLuint mainShader = createShaderProgram("Shaders/sample.vert", "Shaders/sample.frag");
    GLuint skyboxShader = createShaderProgram("Shaders/skybox.vert", "Shaders/skybox.frag");

    GLuint brickTex = loadTexture2D("3D/brickwall.jpg", true);
    GLuint normalTex = loadTexture2D("3D/brickwall_normal.jpg", true);
    GLuint yaeTex = loadTexture2D("3D/yae.png", true);

    GLuint planeVAO, planeVBO, planeEBO;
    size_t planeIndexCount = 0;

    if (!loadPlaneMeshWithTangent("3D/plane.obj", planeVAO, planeVBO, planeEBO, planeIndexCount)) {
        glfwTerminate();
        return -1;
    }

	// Set up skybox VAO and VBO
    GLuint skyVAO, skyVBO, skyEBO;
    glGenVertexArrays(1, &skyVAO);
    glGenBuffers(1, &skyVBO);
    glGenBuffers(1, &skyEBO);

    glBindVertexArray(skyVAO);

    glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyboxIndices), skyboxIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
	// Load cubemap textures for skybox
    std::vector<std::string> faces{
        "Skybox/rainbow_rt.png",
        "Skybox/rainbow_lf.png",
        "Skybox/rainbow_up.png",
        "Skybox/rainbow_dn.png",
        "Skybox/rainbow_ft.png",
        "Skybox/rainbow_bk.png"
    };
    GLuint cubemapTexture = loadCubemap(faces);

    glUseProgram(mainShader);
    glUniform1i(glGetUniformLocation(mainShader, "tex0"), 0);
    glUniform1i(glGetUniformLocation(mainShader, "norm_tex"), 1);
    glUniform1i(glGetUniformLocation(mainShader, "yae_tex"), 2);

    glUseProgram(skyboxShader);
    glUniform1i(glGetUniformLocation(skyboxShader, "skybox"), 0);

    Model3D planeModel;
    planeModel.position = glm::vec3(0.0f);
    planeModel.scale = glm::vec3(1.0f);
	// Camera and light setup
    glm::vec3 eye(0.0f, 0.0f, 3.0f);
    glm::vec3 center(0.0f, 0.0f, 0.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);

    glm::vec3 lightPos(-2.0f, 2.0f, 2.0f);
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
    float lightIntensity = 1.2f;
	
    // Main render loop
    while (!glfwWindowShouldClose(window))
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = glm::lookAt(eye, center, up);
        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),
            (float)SCR_WIDTH / (float)SCR_HEIGHT,
            0.1f,
            100.0f
        );

        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);

        glUseProgram(skyboxShader);
        glm::mat4 skyView = glm::mat4(glm::mat3(view));
        glUniformMatrix4fv(glGetUniformLocation(skyboxShader, "view"), 1, GL_FALSE, glm::value_ptr(skyView));
        glUniformMatrix4fv(glGetUniformLocation(skyboxShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(skyVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);

        // Vertical spin
        planeModel.rotation.x = (float)glfwGetTime() * 35.0f;
        planeModel.rotation.y = 0.0f;
        planeModel.rotation.z = 0.0f;

        glm::mat4 model = planeModel.getMatrix();

        glUseProgram(mainShader);
        glUniformMatrix4fv(glGetUniformLocation(mainShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(mainShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(mainShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glUniform3fv(glGetUniformLocation(mainShader, "pointLightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(mainShader, "pointLightColor"), 1, glm::value_ptr(lightColor));
        glUniform1f(glGetUniformLocation(mainShader, "pointLightIntensity"), lightIntensity);
        glUniform3fv(glGetUniformLocation(mainShader, "viewPos"), 1, glm::value_ptr(eye));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, brickTex);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normalTex);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, yaeTex);

        glBindVertexArray(planeVAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)planeIndexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

	// Cleanup resources
    glDeleteVertexArrays(1, &planeVAO);
    glDeleteBuffers(1, &planeVBO);
    glDeleteBuffers(1, &planeEBO);

    glDeleteVertexArrays(1, &skyVAO);
    glDeleteBuffers(1, &skyVBO);
    glDeleteBuffers(1, &skyEBO);

    glDeleteTextures(1, &brickTex);
    glDeleteTextures(1, &normalTex);
    glDeleteTextures(1, &yaeTex);
    glDeleteTextures(1, &cubemapTexture);

    glDeleteProgram(mainShader);
    glDeleteProgram(skyboxShader);

    glfwTerminate();
    return 0;
}