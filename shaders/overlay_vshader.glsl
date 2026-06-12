#version 410 core

layout(location = 0) in vec2 vPosition;
layout(location = 1) in vec3 vColor;

out vec3 fragColor;

uniform vec2 viewportSize;

void main()
{
    vec2 ndc = vec2(
        (vPosition.x / viewportSize.x) * 2.0 - 1.0,
        1.0 - (vPosition.y / viewportSize.y) * 2.0
    );
    fragColor = vColor;
    gl_Position = vec4(ndc, 0.0, 1.0);
}
