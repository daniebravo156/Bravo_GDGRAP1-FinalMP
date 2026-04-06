#version 330 core

// Fragment shader for rendering the UI overlay with a binocular effect
in vec2 TexCoord;

out vec4 FragColor;

uniform float screenWidth;
uniform float screenHeight;

void main()
{
    vec2 p = TexCoord * 2.0 - 1.0;

    float aspect = screenWidth / screenHeight;
    p.x *= aspect;

    vec2 leftCenter = vec2(-0.42 * aspect, 0.0);
    vec2 rightCenter = vec2(0.42 * aspect, 0.0);

    float radius = 0.72;

    bool inLeft = distance(p, leftCenter) < radius;
    bool inRight = distance(p, rightCenter) < radius;

    bool insideLens = inLeft || inRight;

    bool centerBar = abs(p.x) < 0.025 * aspect;
    bool crossLine = abs(p.y) < 0.006 && abs(p.x) < 0.08 * aspect;

    if (insideLens && !centerBar && !crossLine) {
        discard;
    }

    FragColor = vec4(0.0, 0.0, 0.0, 1.0);
}