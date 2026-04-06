#define TINYOBJLOADER_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#include "Headers/Utils.h"
#include "Headers/Shader.h"
#include "Headers/Camera.h"
#include "Headers/Light.h"
#include "Headers/Model.h"
#include "Headers/Player.h"
#include "Headers/WorldObject.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <string>

// Globals
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
const int GUNDAM_COUNT = 6;

int activeCamera = 1;
// 1 = 3rd person
// 2 = binocular
// 3 = top view

bool cursorEnabled = false;
bool firstMouse = true;

float lastX = SCR_WIDTH * 0.5f;
float lastY = SCR_HEIGHT * 0.5f;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

int pointLightMode = 1;
float pointLightLevels[3] = { 0.35f, 0.8f, 1.3f };

ThirdPersonCamera thirdCam(18.0f, glm::vec3(0.0f, 2.5f, 0.0f));
BinocularCamera binoCam(glm::vec3(0.0f));
OrthoCamera orthoCam(glm::vec3(0.0f));

Player player;

PointLight pLight(glm::vec3(0.0f, 2.0f, 5.0f), glm::vec3(1.0f, 0.95f, 0.85f), 0.8f);
DirectionLight dLight(glm::vec3(-0.3f, -1.0f, -0.2f), glm::vec3(0.45f, 0.5f, 0.7f), 0.7f);

// Cursor
void updateCursorMode(GLFWwindow* window) {
    if (cursorEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        firstMouse = true;
    }
}

// Input
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_TAB) {
            cursorEnabled = !cursorEnabled;
            updateCursorMode(window);
        }

        if (key == GLFW_KEY_1) {
            if (activeCamera == 1) {
                activeCamera = 2;
                binoCam.setPosition(player.getBinocularPosition());
                binoCam.setAngles(player.yaw, 0.0f);
            }
            else {
                activeCamera = 1;
                firstMouse = true;
            }
        }

        if (key == GLFW_KEY_2) {
            activeCamera = 3;
        }

        if (key == GLFW_KEY_F) {
            pointLightMode = (pointLightMode + 1) % 3;
            pLight.intensity = pointLightLevels[pointLightMode];
        }
    }
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    if (!cursorEnabled) {
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

            thirdCam.updateAngles(xoffset, yoffset);
        }
    }
}

void processInput(GLFWwindow* window) {
    if (!cursorEnabled) {
        // Tank moves only in 3rd person
        if (activeCamera == 1) {
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                player.moveForward(deltaTime);
            }
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                player.moveBackward(deltaTime);
            }
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                player.turnLeft(deltaTime);
            }
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                player.turnRight(deltaTime);
            }
        }

        // Binocular controls
        if (activeCamera == 2) {
            float lookSpeed = 50.0f * deltaTime;
            float zoomSpeed = 30.0f * deltaTime;

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                binoCam.rotate(0.0f, lookSpeed);
            }
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                binoCam.rotate(0.0f, -lookSpeed);
            }
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                binoCam.rotate(-lookSpeed, 0.0f);
            }
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                binoCam.rotate(lookSpeed, 0.0f);
            }
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
                binoCam.zoom(-zoomSpeed);
            }
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
                binoCam.zoom(zoomSpeed);
            }
        }

        // Top view pan
        if (activeCamera == 3) {
            float panSpeed = 25.0f * deltaTime;

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                orthoCam.pan(0.0f, -panSpeed);
            }
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                orthoCam.pan(0.0f, panSpeed);
            }
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                orthoCam.pan(-panSpeed, 0.0f);
            }
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                orthoCam.pan(panSpeed, 0.0f);
            }
        }
    }
}

int main() {
    int appStatus = 0;
    bool programReady = false;
    GLFWwindow* window = NULL;

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "GDGRAP1 Final - Eldens Hill", NULL, NULL);

    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        appStatus = -1;
    }

    if (appStatus == 0) {
        glfwMakeContextCurrent(window);

        if (!gladLoadGL()) {
            std::cout << "Failed to initialize GLAD" << std::endl;
            appStatus = -1;
        }
        else {
            programReady = true;
        }
    }

    if (programReady) {
        glfwSetKeyCallback(window, key_callback);
        glfwSetCursorPosCallback(window, mouse_callback);
        updateCursorMode(window);

        glEnable(GL_DEPTH_TEST);

        // Shaders
        Shader objectShader("Shaders/object.vert", "Shaders/object.frag");
        Shader skyboxShader("Shaders/skybox.vert", "Shaders/skybox.frag");
        Shader uiShader("Shaders/ui.vert", "Shaders/ui.frag");

        // Textures
        GLuint groundTexture = loadTexture("3D/field.jpg", true);
        GLuint tankDiffuseTexture = loadTexture("3D/tank_diffuse.PNG", true);
        GLuint tankNormalTexture = loadTexture("3D/tank_normal.png", true);

        objectShader.use();
        objectShader.setInt("tex0", 0);
        objectShader.setInt("norm_tex", 1);

        skyboxShader.use();
        skyboxShader.setInt("skybox", 0);
        skyboxShader.setBool("nightVisionMode", false);

        // Tank
        GLuint tankVAO = 0;
        GLuint tankVBO = 0;
        GLuint tankEBO = 0;
        size_t tankIndices = 0;
        bool tankLoaded = false;

        tankLoaded = loadMeshNormalMapped("3D/tank.obj", tankVAO, tankVBO, tankEBO, tankIndices);

        if (tankLoaded) {
            player.model.position = glm::vec3(0.0f, 0.0f, 0.0f);
            player.model.scale = glm::vec3(1.0f);
            player.visualYawOffset = 90.0f;
            player.syncRotation();
        }
        else {
            std::cout << "Tank failed to load." << std::endl;
            appStatus = -1;
        }

        // Ground
        GLuint groundVAO = 0;
        GLuint groundVBO = 0;
        GLuint groundEBO = 0;
        size_t groundIndices = 0;
        createGroundPlane(groundVAO, groundVBO, groundEBO, groundIndices);

        Model3D groundModel;
        groundModel.position = glm::vec3(0.0f);

        // Skybox geometry
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

        GLuint skyboxVAO = 0;
        GLuint skyboxVBO = 0;
        GLuint skyboxEBO = 0;
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
			"Skybox/skyboxRT.png", //x+
			"Skybox/skyboxLF.png", //x-
			"Skybox/skyboxUP.png", //y+
			"Skybox/skyboxDN.png", //y-
			"Skybox/skyboxBK.png", //z+
			"Skybox/skyboxFT.png" //z-
            
        };

        GLuint cubemapTexture = loadCubemap(skyboxFaces);

        // Gundam asset paths
        std::string gundamObjPaths[GUNDAM_COUNT] = {
            "3D/Gundams/gundam1/gundam1.obj",
            "3D/Gundams/gundam2/gundam2.obj",
            "3D/Gundams/gundam3/gundam3.obj",
            "3D/Gundams/gundam4/gundam4.obj",
            "3D/Gundams/gundam5/gundam5.obj",
            "3D/Gundams/gundam6/gundam6.obj"
        };

        std::string gundamDiffusePaths[GUNDAM_COUNT] = {
            "3D/Gundams/gundam1/gundam1_D.png",
            "3D/Gundams/gundam2/gundam2_D.png",
            "3D/Gundams/gundam3/gundam3_D.png",
            "3D/Gundams/gundam4/gundam4_D.png",
            "3D/Gundams/gundam5/gundam5_D.png",
            "3D/Gundams/gundam6/gundam6_D.png"
        };

        std::string gundamNormalPaths[GUNDAM_COUNT] = {
            "3D/Gundams/gundam1/gundam1_N.png",
            "3D/Gundams/gundam2/gundam2_N.png",
            "3D/Gundams/gundam3/gundam3_N.png",
            "3D/Gundams/gundam4/gundam4_N.png",
            "3D/Gundams/gundam5/gundam5_N.png",
            "3D/Gundams/gundam6/gundam6_N.png"
        };

        // Gundam formation
		float gundamRingRadius = 62.0f; //distance from center/player
		float gundamY = -0.20f; //height on the ground
        float gundamScale = 9.0f; //size
        float gundamFacingOffset = 90.0f; //face me

        // Gundam assets
        std::vector<WorldObjectAsset> gundamAssets;
        std::vector<WorldObjectInstance> gundamObjects;

        if (appStatus == 0) {
            gundamAssets.resize(GUNDAM_COUNT);
            gundamObjects.resize(GUNDAM_COUNT);

            int i = 0;
            bool loadOk = true;

            while (i < GUNDAM_COUNT && loadOk) {
                gundamAssets[i].diffuseTexture = loadTexture(gundamDiffusePaths[i].c_str(), true);
                gundamAssets[i].normalTexture = loadTexture(gundamNormalPaths[i].c_str(), true);

                loadOk = loadMeshNormalMapped(
                    gundamObjPaths[i].c_str(),
                    gundamAssets[i].VAO,
                    gundamAssets[i].VBO,
                    gundamAssets[i].EBO,
                    gundamAssets[i].indexCount
                );

                if (!loadOk) {
                    std::cout << "Gundam asset failed to load at index " << i << std::endl;
                    appStatus = -1;
                }

                i++;
            }
        }

        // Gundam objects
        if (appStatus == 0) {
            int i = 0;
            float angleStep = 360.0f / (float)GUNDAM_COUNT;

            while (i < GUNDAM_COUNT) {
                float angleDeg = angleStep * (float)i;
                float angleRad = glm::radians(angleDeg);

                float posX = cos(angleRad) * gundamRingRadius;
                float posZ = sin(angleRad) * gundamRingRadius;

                glm::vec3 toPlayer = glm::normalize(glm::vec3(-posX, 0.0f, -posZ));
                float faceYaw = glm::degrees(atan2(toPlayer.z, toPlayer.x));

                gundamObjects[i].model.position = glm::vec3(posX, gundamY, posZ);
                gundamObjects[i].model.rotation = glm::vec3(0.0f, -faceYaw + gundamFacingOffset, 0.0f);
                gundamObjects[i].model.scale = glm::vec3(gundamScale);
                gundamObjects[i].useNormalMap = true;

                i++;
            }
        }

        // Binocular overlay quad
        float uiVertices[] = {
            // pos      // uv
            -1.0f, -1.0f,  0.0f, 0.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
             1.0f,  1.0f,  1.0f, 1.0f,
            -1.0f,  1.0f,  0.0f, 1.0f
        };

        unsigned int uiIndices[] = {
            0, 1, 2,
            0, 2, 3
        };

        GLuint uiVAO = 0;
        GLuint uiVBO = 0;
        GLuint uiEBO = 0;
        glGenVertexArrays(1, &uiVAO);
        glGenBuffers(1, &uiVBO);
        glGenBuffers(1, &uiEBO);

        glBindVertexArray(uiVAO);

        glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(uiVertices), uiVertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, uiEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uiIndices), uiIndices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        while (appStatus == 0 && !glfwWindowShouldClose(window)) {
            float currentFrame = (float)glfwGetTime();
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            processInput(window);

            thirdCam.setTarget(player.getCameraTarget());
            binoCam.setPosition(player.getBinocularPosition());
            pLight.position = player.getFrontLightPos();

            glClearColor(0.03f, 0.03f, 0.05f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glm::mat4 view = glm::mat4(1.0f);
            glm::mat4 projection = glm::mat4(1.0f);
            bool nightVisionMode = false;

            if (activeCamera == 1) {
                view = thirdCam.getViewMatrix();
                projection = thirdCam.getProjectionMatrix();
                nightVisionMode = false;
            }
            else {
                if (activeCamera == 2) {
                    view = binoCam.getViewMatrix();
                    projection = binoCam.getProjectionMatrix();
                    nightVisionMode = true;
                }
                else {
                    view = orthoCam.getViewMatrix();
                    projection = orthoCam.getProjectionMatrix();
                    nightVisionMode = false;
                }
            }

            // Skybox
            glDepthFunc(GL_LEQUAL);
            glDepthMask(GL_FALSE);

            skyboxShader.use();
            skyboxShader.setBool("nightVisionMode", nightVisionMode);

            glm::mat4 skyView = glm::mat4(glm::mat3(view));
            skyboxShader.setMat4("view", skyView);
            skyboxShader.setMat4("projection", projection);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

            glBindVertexArray(skyboxVAO);
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LESS);

            // Objects
            objectShader.use();
            objectShader.setMat4("view", view);
            objectShader.setMat4("projection", projection);

            objectShader.setVec3("pointLightPos", pLight.position);
            objectShader.setVec3("pointLightColor", pLight.color);
            objectShader.setFloat("pointLightIntensity", pLight.intensity);

            objectShader.setVec3("dirLightDirection", dLight.direction);
            objectShader.setVec3("dirLightColor", dLight.color);
            objectShader.setFloat("dirLightIntensity", dLight.intensity);

            objectShader.setBool("nightVisionMode", nightVisionMode);

            // Ground
            objectShader.setBool("useNormalMap", false);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, groundTexture);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, tankNormalTexture);

            groundModel.draw(objectShader.ID, groundVAO, (int)groundIndices);

            // Tank
            if (activeCamera != 2) {
                objectShader.setBool("useNormalMap", true);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, tankDiffuseTexture);

                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, tankNormalTexture);

                player.draw(objectShader.ID, tankVAO, (int)tankIndices);
            }

            // Gundam objects
            if (appStatus == 0) {
                int i = 0;

                while (i < GUNDAM_COUNT) {
                    drawWorldObject(objectShader, gundamAssets[i], gundamObjects[i]);
                    i++;
                }
            }

            // Binocular overlay
            if (activeCamera == 2) {
                glDisable(GL_DEPTH_TEST);

                uiShader.use();
                uiShader.setFloat("screenWidth", (float)SCR_WIDTH);
                uiShader.setFloat("screenHeight", (float)SCR_HEIGHT);

                glBindVertexArray(uiVAO);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                glEnable(GL_DEPTH_TEST);
            }

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

    if (window != NULL) {
        glfwDestroyWindow(window);
    }

    glfwTerminate();
    return appStatus;
}