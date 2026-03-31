#version 410 core

in vec3 fragPos;
in vec3 fragNormal;

uniform vec3 objectColor;
uniform vec3 lightDir;     // direction TO the light (normalized)
uniform vec3 lightColor;
uniform vec3 viewPos;

out vec4 fColor;

void main()
{
    vec3 norm = normalize(fragNormal);

    // Ambient
    float ambientStrength = 0.15;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular (Blinn-Phong)
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfDir), 0.0), 64.0);
    vec3 specular = 0.4 * spec * lightColor;

    vec3 result = (ambient + diffuse + specular) * objectColor;
    fColor = vec4(result, 1.0);
}
