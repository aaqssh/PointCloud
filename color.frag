#version 450

layout(location = 0) in float v_intensity;
layout(location = 0) out vec4 fragColor;

void main() {
    // Map height (intensity) to grayscale white
    fragColor = vec4(vec3(v_intensity), 1.0);
}