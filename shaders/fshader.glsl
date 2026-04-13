#version 410 core

in vec3 fragPos;
in vec3 fragNormal;
in vec2 fragTexCoord;

uniform vec3 objectColor;
uniform vec3 lightDir;     // direction TO the light (normalized)
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform sampler2D buildingTex;
uniform bool useTexture;
uniform float ambientStrength;
uniform float specularStrength;
uniform float shininess;

out vec4 fColor;

void main()
{
    vec3 norm = normalize(fragNormal);

    // Ambient
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular (Blinn-Phong)
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 baseColor = objectColor;
    if (useTexture) {
        baseColor *= texture(buildingTex, fragTexCoord).rgb;
    }

    vec3 result = (ambient + diffuse + specular) * baseColor;
    fColor = vec4(result, 1.0);
}
