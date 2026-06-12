#version 410 core

in vec3 fragPos;
in vec3 fragNormal;
in vec2 fragUV;

uniform vec3 objectColor;
uniform vec3 lightDir;     // direction TO the light (normalized)
uniform vec3 lightColor;
uniform vec3 ambientTint;
uniform vec3 viewPos;
uniform sampler2D diffuseMap;
uniform bool hasTexture;
uniform int materialMode;  // 0 = plain, 1 = facade, 2 = roof, 3 = road/path, 4 = rectorate facade, 5 = grass, 6 = stone, 11 = water, 12 = stair stone, 13 = fence, 14 = foliage, 15 = trunk
uniform float nightBlend;

out vec4 fColor;

float rectMask(vec2 p, vec2 minCorner, vec2 maxCorner)
{
    vec2 feather = vec2(0.08);
    vec2 insideMin = smoothstep(minCorner, minCorner + feather, p);
    vec2 insideMax = 1.0 - smoothstep(maxCorner - feather, maxCorner, p);
    return insideMin.x * insideMin.y * insideMax.x * insideMax.y;
}

void addWindow(vec2 p,
               vec2 center,
               vec2 halfSize,
               inout float trimMask,
               inout float glassMask,
               inout float mullionMask)
{
    vec2 trimPad = vec2(0.28, 0.34);
    float trim = rectMask(p, center - halfSize - trimPad, center + halfSize + trimPad);
    float glass = rectMask(p, center - halfSize, center + halfSize);
    float verticalBar = rectMask(p,
                                 vec2(center.x - 0.055, center.y - halfSize.y),
                                 vec2(center.x + 0.055, center.y + halfSize.y));
    float horizontalBar = rectMask(p,
                                   vec2(center.x - halfSize.x, center.y - 0.055),
                                   vec2(center.x + halfSize.x, center.y + 0.055));

    trimMask = max(trimMask, trim);
    glassMask = max(glassMask, glass);
    mullionMask = max(mullionMask, glass * max(verticalBar, horizontalBar));
}

void main()
{
    vec3 norm = normalize(fragNormal);

    // Ambient
    float ambientStrength = 0.20;
    vec3 skyAmbient = mix(ambientTint * 0.58, ambientTint * 1.10, norm.y * 0.5 + 0.5);
    vec3 groundBounce = vec3(0.18, 0.15, 0.12) * (0.5 - 0.5 * norm.y);
    vec3 ambient = ambientStrength * (skyAmbient + groundBounce);

    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    diff = smoothstep(0.0, 1.0, diff);
    vec3 diffuse = diff * lightColor;

    // Specular (Blinn-Phong) — lower strength for textured surfaces (terrain)
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 halfDir = normalize(lightDir + viewDir);
    float specStrength = hasTexture ? 0.08 : 0.3;
    float shininess   = hasTexture ? 16.0 : 64.0;
    float spec = pow(max(dot(norm, halfDir), 0.0), shininess);
    vec3 specular = specStrength * spec * lightColor * mix(1.0, 0.72, nightBlend);

    vec3 baseColor = objectColor;
    if (materialMode == 4 && abs(norm.y) < 0.55) {
        float floorBand = smoothstep(0.16, 0.0, abs(fragPos.y - 78.8)) +
                          smoothstep(0.16, 0.0, abs(fragPos.y - 84.4));
        baseColor = mix(baseColor, baseColor * 0.82, clamp(floorBand, 0.0, 1.0) * 0.45);

        float trimMask = 0.0;
        float glassMask = 0.0;
        float mullionMask = 0.0;

        if (abs(norm.x) > abs(norm.z)) {
            vec2 p = vec2(fragPos.z, fragPos.y);
            vec2 sideWindow = vec2(0.86, 1.12);
            addWindow(p, vec2(66.4, 77.2), sideWindow, trimMask, glassMask, mullionMask);
            addWindow(p, vec2(72.3, 77.2), sideWindow, trimMask, glassMask, mullionMask);
            addWindow(p, vec2(78.2, 77.2), sideWindow, trimMask, glassMask, mullionMask);
            addWindow(p, vec2(66.4, 82.4), sideWindow, trimMask, glassMask, mullionMask);
            addWindow(p, vec2(72.3, 82.4), sideWindow, trimMask, glassMask, mullionMask);
            addWindow(p, vec2(78.2, 82.4), sideWindow, trimMask, glassMask, mullionMask);

            addWindow(p, vec2(106.1, 77.2), sideWindow, trimMask, glassMask, mullionMask);
            addWindow(p, vec2(112.0, 77.2), sideWindow, trimMask, glassMask, mullionMask);
            addWindow(p, vec2(117.9, 77.2), sideWindow, trimMask, glassMask, mullionMask);
            addWindow(p, vec2(106.1, 82.4), sideWindow, trimMask, glassMask, mullionMask);
            addWindow(p, vec2(112.0, 82.4), sideWindow, trimMask, glassMask, mullionMask);
            addWindow(p, vec2(117.9, 82.4), sideWindow, trimMask, glassMask, mullionMask);

            vec2 towerWindow = vec2(0.64, 1.85);
            addWindow(p, vec2(87.7, 90.4), towerWindow, trimMask, glassMask, mullionMask);
            addWindow(p, vec2(92.2, 90.4), towerWindow, trimMask, glassMask, mullionMask);
            addWindow(p, vec2(96.7, 90.4), towerWindow, trimMask, glassMask, mullionMask);
        } else {
            vec2 p = vec2(fragPos.x, fragPos.y);
            vec2 sideWindow = vec2(0.62, 1.05);
            addWindow(p, vec2(-57.8, 77.4), sideWindow, trimMask, glassMask, mullionMask);
            addWindow(p, vec2(-48.6, 77.4), sideWindow, trimMask, glassMask, mullionMask);
            addWindow(p, vec2(-57.8, 82.2), sideWindow, trimMask, glassMask, mullionMask);
            addWindow(p, vec2(-48.6, 82.2), sideWindow, trimMask, glassMask, mullionMask);
        }

        vec3 trimColor = vec3(0.58, 0.53, 0.45);
        vec3 glassColor = vec3(0.055, 0.075, 0.080);
        vec3 mullionColor = vec3(0.18, 0.12, 0.08);

        float frameOnly = clamp(trimMask - glassMask, 0.0, 1.0);
        baseColor = mix(baseColor, trimColor, frameOnly * 0.82);
        baseColor = mix(baseColor, glassColor, glassMask * 0.92);
        baseColor = mix(baseColor, mullionColor, mullionMask * 0.98);
    } else if (materialMode == 5) {
        float bladeNoise = 0.045 * sin(fragPos.x * 1.7) +
                           0.030 * sin(fragPos.z * 2.3) +
                           0.018 * sin((fragPos.x + fragPos.z) * 5.1);
        float stripe = 0.5 + 0.5 * sin(fragPos.z * 0.85);
        baseColor = clamp(baseColor + vec3(bladeNoise * 0.55, bladeNoise, bladeNoise * 0.35), 0.0, 1.0);
        baseColor = mix(baseColor, baseColor * vec3(0.92, 1.08, 0.88), stripe * 0.20);
    } else if (materialMode == 6) {
        float grain = 0.035 * sin(fragPos.x * 0.9) + 0.025 * sin(fragPos.z * 1.3);
        float blockLineY = smoothstep(0.00, 0.018, fract(fragPos.y * 1.35)) *
                           (1.0 - smoothstep(0.040, 0.075, fract(fragPos.y * 1.35)));
        float blockLineX = smoothstep(0.00, 0.016, fract(fragPos.x * 0.33)) *
                           (1.0 - smoothstep(0.035, 0.070, fract(fragPos.x * 0.33)));
        baseColor = clamp(baseColor + vec3(grain), 0.0, 1.0);
        baseColor = mix(baseColor, baseColor * 0.70, clamp(blockLineY + blockLineX * 0.35, 0.0, 1.0) * 0.35);
    } else if (materialMode == 11) {
        float ripple = 0.5 + 0.5 * sin(fragPos.x * 3.4 + fragPos.z * 2.8);
        float shimmer = 0.5 + 0.5 * sin(fragPos.z * 4.7 - fragPos.x * 2.1);
        vec3 deep = baseColor * vec3(0.62, 0.88, 1.10);
        vec3 bright = vec3(0.56, 0.80, 0.96);
        baseColor = mix(deep, bright, ripple * 0.40 + shimmer * 0.20);
    } else if (materialMode == 12) {
        float grain = 0.045 * sin(fragPos.x * 1.2) + 0.030 * sin(fragPos.z * 1.6);
        float seamY = smoothstep(0.00, 0.020, fract(fragPos.y * 1.10)) *
                      (1.0 - smoothstep(0.035, 0.080, fract(fragPos.y * 1.10)));
        float seamX = smoothstep(0.00, 0.018, fract((fragPos.x + fragPos.z) * 0.18)) *
                      (1.0 - smoothstep(0.040, 0.090, fract((fragPos.x + fragPos.z) * 0.18)));
        float edgeLight = pow(max(dot(norm, normalize(vec3(-0.30, 0.90, 0.40))), 0.0), 2.0);
        baseColor = clamp(baseColor + vec3(grain), 0.0, 1.0);
        baseColor = mix(baseColor, baseColor * 0.62, clamp(seamY + seamX, 0.0, 1.0) * 0.45);
        baseColor += vec3(0.10, 0.08, 0.06) * edgeLight;
    } else if (materialMode == 13) {
        float meshX = abs(fract(fragPos.x * 2.6) - 0.5);
        float meshY = abs(fract(fragPos.y * 3.0) - 0.5);
        float wire = 1.0 - smoothstep(0.18, 0.26, min(meshX, meshY));
        baseColor = mix(baseColor * 0.45, baseColor * 1.18, wire);
    } else if (materialMode == 14) {
        float leafNoise = 0.06 * sin(fragPos.x * 1.6) + 0.05 * sin(fragPos.z * 1.4) + 0.03 * sin(fragPos.y * 2.1);
        float sunKiss = pow(max(dot(norm, normalize(vec3(-0.25, 0.95, 0.30))), 0.0), 1.4);
        baseColor = clamp(baseColor + vec3(leafNoise * 0.4, leafNoise, leafNoise * 0.25), 0.0, 1.0);
        baseColor = mix(baseColor, baseColor * vec3(1.12, 1.16, 0.92), sunKiss * 0.24);
    } else if (materialMode == 15) {
        float bark = 0.06 * sin(fragPos.y * 4.5) + 0.03 * sin((fragPos.x + fragPos.z) * 3.2);
        baseColor = clamp(baseColor + vec3(bark * 0.55, bark * 0.35, bark * 0.18), 0.0, 1.0);
    } else if (hasTexture) {
        // Linearize sRGB texture for correct lighting
        vec3 texel = texture(diffuseMap, fragUV).rgb;
        texel = pow(texel, vec3(2.2));
        baseColor *= texel;
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
        float seamX = smoothstep(0.00, 0.035, fract(fragPos.x * 0.24)) *
                      (1.0 - smoothstep(0.055, 0.11, fract(fragPos.x * 0.24)));
        float seamZ = smoothstep(0.00, 0.035, fract(fragPos.z * 0.24)) *
                      (1.0 - smoothstep(0.055, 0.11, fract(fragPos.z * 0.24)));
        float dust = 0.03 * sin(fragPos.x * 0.07 + fragPos.z * 0.09);
        baseColor = clamp(baseColor + vec3(paving + dust), 0.0, 1.0);
        baseColor = mix(baseColor, baseColor * 0.72, clamp(seamX + seamZ * 0.8, 0.0, 1.0) * 0.22);
    }

    vec3 result = (ambient + diffuse + specular) * baseColor;

    // Gamma correction (linear -> sRGB output)
    result = pow(result, vec3(1.0 / 2.2));

    fColor = vec4(result, 1.0);
}
