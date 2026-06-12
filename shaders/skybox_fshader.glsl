#version 410 core

in vec3 texDir;

out vec4 fColor;

uniform samplerCube skybox;
uniform float blendFactor;
uniform vec3 skyTint;

float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float noise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

float fbm(vec2 p)
{
    float value = 0.0;
    float amp = 0.5;
    for (int i = 0; i < 4; ++i) {
        value += amp * noise(p);
        p *= 2.03;
        amp *= 0.5;
    }
    return value;
}

void main()
{
    vec3 dir = normalize(texDir);
    vec3 cube = texture(skybox, dir).rgb;
    float horizon = clamp(dir.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 day = cube * vec3(1.08, 1.03, 0.97);
    vec3 night = mix(cube * vec3(0.16, 0.22, 0.38), skyTint, 0.45);
    vec3 color = mix(day, night, blendFactor);

    vec2 skyUV = dir.xz / max(abs(dir.y) + 0.25, 0.25);
    float cloud = smoothstep(0.48, 0.72, fbm(skyUV * 2.2 + vec2(0.0, 0.03)));
    cloud *= smoothstep(0.12, 0.55, dir.y);
    vec3 cloudColor = mix(vec3(1.0), vec3(0.62, 0.66, 0.74), blendFactor);
    color = mix(color, cloudColor, cloud * mix(0.42, 0.18, blendFactor));

    vec3 sunDir = normalize(vec3(-0.35, 0.85, 0.45));
    vec3 moonDir = normalize(vec3(0.28, 0.42, -0.86));
    float sunDisc = pow(max(dot(dir, sunDir), 0.0), 720.0);
    float sunGlow = pow(max(dot(dir, sunDir), 0.0), 18.0);
    float moonDisc = pow(max(dot(dir, moonDir), 0.0), 900.0);
    float moonGlow = pow(max(dot(dir, moonDir), 0.0), 26.0);

    color += vec3(1.00, 0.92, 0.78) * sunDisc * (1.0 - blendFactor);
    color += vec3(1.00, 0.76, 0.42) * sunGlow * 0.32 * (1.0 - blendFactor);
    color += vec3(0.88, 0.92, 1.00) * moonDisc * blendFactor;
    color += vec3(0.45, 0.53, 0.70) * moonGlow * 0.18 * blendFactor;

    color = mix(color, color * vec3(1.08, 1.00, 0.92), horizon * 0.12);
    fColor = vec4(color, 1.0);
}
