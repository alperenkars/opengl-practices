#version 410 core

in vec3 fragPos;
in vec3 fragNormal;
in vec2 fragUV;

uniform vec3 objectColor;
uniform vec3 lightDir;     // direction TO the light (normalized)
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform sampler2D diffuseMap;
uniform bool hasTexture;

out vec4 fColor;

void main()
{
    vec3 norm = normalize(fragNormal);

    // Ambient
    float ambientStrength = 0.20;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular (Blinn-Phong) — lower strength for textured surfaces (terrain)
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 halfDir = normalize(lightDir + viewDir);
    float specStrength = hasTexture ? 0.08 : 0.3;
    float shininess   = hasTexture ? 16.0 : 64.0;
    float spec = pow(max(dot(norm, halfDir), 0.0), shininess);
    vec3 specular = specStrength * spec * lightColor;

    vec3 baseColor = objectColor;
    if (hasTexture) {
        // Linearize sRGB texture for correct lighting
        vec3 texel = texture(diffuseMap, fragUV).rgb;
        texel = pow(texel, vec3(2.2));
        baseColor *= texel;
    }

    vec3 result = (ambient + diffuse + specular) * baseColor;

    // Gamma correction (linear -> sRGB output)
    result = pow(result, vec3(1.0 / 2.2));

    fColor = vec4(result, 1.0);
}
