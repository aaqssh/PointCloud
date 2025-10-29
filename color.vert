#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;  // Unused here, can be omitted if not needed

layout(location = 0) out float v_intensity;

layout(set = 0, binding = 0) uniform buf {
    mat4 mvp;
} ubuf;

void main() {
    gl_Position = ubuf.mvp * vec4(position, 1.0);
    gl_PointSize = 3.0;

    // Normalize Z to intensity range [0.0, 1.0]
    float intensity = clamp((position.z + 1.0) / 2.0, 0.0, 1.0);
    v_intensity = intensity;
}