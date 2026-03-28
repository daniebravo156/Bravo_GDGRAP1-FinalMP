#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec2 TexCoord;
in mat3 TBN;

uniform sampler2D tex0;
uniform sampler2D norm_tex;
uniform sampler2D yae_tex;

uniform vec3 pointLightPos;
uniform vec3 pointLightColor;
uniform float pointLightIntensity;
uniform vec3 viewPos;

// blends a brick texture with a yae texture based on the alpha of the yae texture
void main()
{
    vec2 uv = vec2(1.0 - TexCoord.y, TexCoord.x);

    vec4 brick = texture(tex0, uv);
    vec4 yae = texture(yae_tex, uv);

    vec3 sampledNormal = texture(norm_tex, uv).rgb;
    sampledNormal = normalize(sampledNormal * 2.0 - 1.0);
    vec3 norm = normalize(TBN * sampledNormal);

    vec3 baseColor = mix(brick.rgb, yae.rgb, yae.a);

    vec3 lightDir = normalize(pointLightPos - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    float diff = max(dot(norm, lightDir), 0.0);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);

    vec3 color =
        0.15 * baseColor +
        diff * pointLightColor * pointLightIntensity * baseColor +
        spec * pointLightColor * 0.6;

    float finalAlpha = mix(0.5, 1.0, yae.a);

    FragColor = vec4(color, finalAlpha);
}