#version 330 core

// Vertex shader for rendering objects with normal mapping
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTex;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out vec3 FragPos;
out vec3 WorldNormal;
out vec2 TexCoord;
out mat3 TBN;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = vec3(worldPos);

    mat3 normalMatrix = mat3(transpose(inverse(model)));
    vec3 N = normalize(normalMatrix * aNormal);

    vec3 T = normalize(mat3(model) * aTangent);
    T = normalize(T - dot(T, N) * N);

    vec3 B = normalize(mat3(model) * aBitangent);

    WorldNormal = N;
    TexCoord = aTex;
    TBN = mat3(T, B, N);

    gl_Position = projection * view * worldPos;
}