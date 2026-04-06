#version 330 core
// Fragment shader for rendering the skybox with night vision mode
in vec3 TexCoords;

out vec4 FragColor;

uniform samplerCube skybox;
uniform bool nightVisionMode;

void main()
{
    vec3 color = texture(skybox, TexCoords).rgb;

    if (nightVisionMode) {
        float brightness = dot(color, vec3(0.299, 0.587, 0.114));
        color = vec3(0.08, 1.0, 0.08) * brightness;
    }

    FragColor = vec4(color, 1.0);
}