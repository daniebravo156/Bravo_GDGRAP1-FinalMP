#version 330 core

in vec3 FragPos;
in vec3 WorldNormal;
in vec2 TexCoord;
in mat3 TBN;

out vec4 FragColor;

uniform sampler2D tex0;
uniform sampler2D norm_tex;
uniform bool useNormalMap;
uniform bool nightVisionMode;

uniform vec3 pointLightPos;
uniform vec3 pointLightColor;
uniform float pointLightIntensity;

uniform vec3 dirLightDirection;
uniform vec3 dirLightColor;
uniform float dirLightIntensity;

void main()
{
    vec3 baseColor = texture(tex0, TexCoord).rgb;

    vec3 normal = normalize(WorldNormal);

    if (useNormalMap) {
        vec3 mapNormal = texture(norm_tex, TexCoord).rgb;
        mapNormal = normalize(mapNormal * 2.0 - 1.0);
        normal = normalize(TBN * mapNormal);
    }

    vec3 ambient = 0.15 * baseColor;

    vec3 pointDir = normalize(pointLightPos - FragPos);
    float pointDiff = max(dot(normal, pointDir), 0.0);
    vec3 pointResult = pointDiff * pointLightColor * pointLightIntensity * baseColor;

    vec3 moonDir = normalize(-dirLightDirection);
    float dirDiff = max(dot(normal, moonDir), 0.0);
    vec3 dirResult = dirDiff * dirLightColor * dirLightIntensity * baseColor;

    vec3 finalColor = ambient + pointResult + dirResult;

    if (nightVisionMode) {
        float brightness = dot(finalColor, vec3(0.299, 0.587, 0.114));
        finalColor = vec3(0.08, 1.0, 0.08) * brightness;
    }

    FragColor = vec4(finalColor, 1.0);
}