#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

// Simple shader class to load, compile, and manage vertex and fragment shaders
class Shader {
public:
    GLuint ID = 0;

    Shader(const char* vertexPath, const char* fragmentPath) {
        std::string vertexCode = readFile(vertexPath);
        std::string fragmentCode = readFile(fragmentPath);

        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vShaderCode, NULL);
        glCompileShader(vertexShader);
        checkCompile(vertexShader, "VERTEX");

        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fShaderCode, NULL);
        glCompileShader(fragmentShader);
        checkCompile(fragmentShader, "FRAGMENT");

        ID = glCreateProgram();
        glAttachShader(ID, vertexShader);
        glAttachShader(ID, fragmentShader);
        glLinkProgram(ID);
        checkLink(ID);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    void use() const {
        glUseProgram(ID);
    }

    void setBool(const char* name, bool value) const {
        glUniform1i(glGetUniformLocation(ID, name), (int)value);
    }

    void setInt(const char* name, int value) const {
        glUniform1i(glGetUniformLocation(ID, name), value);
    }

    void setFloat(const char* name, float value) const {
        glUniform1f(glGetUniformLocation(ID, name), value);
    }

    void setVec3(const char* name, const glm::vec3& value) const {
        glUniform3fv(glGetUniformLocation(ID, name), 1, glm::value_ptr(value));
    }

    void setMat4(const char* name, const glm::mat4& mat) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name), 1, GL_FALSE, glm::value_ptr(mat));
    }

private:
    std::string readFile(const char* path) {
        std::ifstream file(path, std::ios::binary);

        if (!file) {
            std::cout << "FAILED TO OPEN FILE: " << path << std::endl;
            return "";
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    void checkCompile(GLuint shader, const char* type) {
        GLint success;
        GLchar infoLog[1024];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cout << type << " SHADER ERROR:\n" << infoLog << std::endl;
        }
    }

    void checkLink(GLuint program) {
        GLint success;
        GLchar infoLog[1024];
        glGetProgramiv(program, GL_LINK_STATUS, &success);

        if (!success) {
            glGetProgramInfoLog(program, 1024, NULL, infoLog);
            std::cout << "PROGRAM LINK ERROR:\n" << infoLog << std::endl;
        }
    }
};