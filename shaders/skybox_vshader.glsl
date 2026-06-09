#version 410 core

layout(location = 0) in vec3 vPosition;

out vec3 texDir;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    texDir = vPosition;
    vec4 clip = projection * view * vec4(vPosition, 1.0);
    gl_Position = clip.xyww;
}
