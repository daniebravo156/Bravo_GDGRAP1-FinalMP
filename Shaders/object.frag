#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D tex0;

uniform vec3 pointLightPos;
uniform vec3 pointLightColor;
uniform float pointLightIntensity;

uniform vec3 dirLightDirection;
uniform vec3 dirLightColor;
uniform float dirLightIntensity;

void main()
{
    vec3 baseColor = texture(tex0, TexCoord).rgb;
    vec3 norm = normalize(Normal);

    vec3 ambient = 0.18 * baseColor;

    vec3 pointDir = normalize(pointLightPos - FragPos);
    float pointDiff = max(dot(norm, pointDir), 0.0);
    vec3 pointLight = pointDiff * pointLightColor * pointLightIntensity * baseColor;

    vec3 moonDir = normalize(-dirLightDirection);
    float dirDiff = max(dot(norm, moonDir), 0.0);
    vec3 dirLight = dirDiff * dirLightColor * dirLightIntensity * baseColor;

    vec3 finalColor = ambient + pointLight + dirLight;
    FragColor = vec4(finalColor, 1.0);
}