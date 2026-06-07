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
uniform int materialMode;  // 0 = plain, 1 = facade, 2 = roof, 3 = road/path

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
    } else if (materialMode == 1 && abs(norm.y) < 0.55) {
        float facadeU = (abs(norm.x) > abs(norm.z)) ? fragPos.z : fragPos.x;
        float floorCoord = fract(fragPos.y * 0.34);
        float bayCoord = fract(facadeU * 0.105);

        float floorSlab = smoothstep(0.00, 0.035, floorCoord) *
                          (1.0 - smoothstep(0.075, 0.12, floorCoord));
        float verticalMullion = smoothstep(0.00, 0.025, bayCoord) *
                                (1.0 - smoothstep(0.055, 0.095, bayCoord));
        float windowX = smoothstep(0.25, 0.31, bayCoord) *
                        (1.0 - smoothstep(0.72, 0.78, bayCoord));
        float windowY = smoothstep(0.36, 0.42, floorCoord) *
                        (1.0 - smoothstep(0.72, 0.80, floorCoord));
        float windowMask = windowX * windowY;

        vec3 slabColor = baseColor * 0.78;
        vec3 mullionColor = baseColor * 0.82;
        vec3 glassColor = vec3(0.10, 0.13, 0.15);

        baseColor = mix(baseColor, slabColor, floorSlab * 0.65);
        baseColor = mix(baseColor, mullionColor, verticalMullion * 0.35);
        baseColor = mix(baseColor, glassColor, windowMask * 0.58);
    } else if (materialMode == 2) {
        vec2 roofUV = vec2(fragPos.x * 0.28, fragPos.z * 0.16);
        float row = fract(roofUV.y);
        float tile = fract(roofUV.x + floor(roofUV.y) * 0.45);

        float rowSeam = smoothstep(0.00, 0.035, row) *
                        (1.0 - smoothstep(0.075, 0.13, row));
        float tileSeam = smoothstep(0.00, 0.025, tile) *
                         (1.0 - smoothstep(0.045, 0.085, tile));
        float ridge = 0.5 + 0.5 * sin(tile * 3.14159);
        float weathering = 0.06 * sin(fragPos.x * 0.37) + 0.04 * sin(fragPos.z * 0.51);

        vec3 clay = mix(baseColor * 0.85, baseColor * 1.18, ridge);
        clay += vec3(weathering, weathering * 0.55, weathering * 0.25);
        clay = mix(clay, clay * 0.55, clamp(rowSeam + tileSeam, 0.0, 1.0));
        baseColor = clamp(clay, 0.0, 1.0);
    } else if (materialMode == 3) {
        float paving = 0.04 * sin(fragPos.x * 0.35) * sin(fragPos.z * 0.35);
        baseColor = clamp(baseColor + vec3(paving), 0.0, 1.0);
    }

    vec3 result = (ambient + diffuse + specular) * baseColor;

    // Gamma correction (linear -> sRGB output)
    result = pow(result, vec3(1.0 / 2.2));

    fColor = vec4(result, 1.0);
}
