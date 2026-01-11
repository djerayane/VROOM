#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

void main() {
    vec4 worldPos = push.model * vec4(inPosition, 1.0);

    // Simple view matrix: move camera back along Z
    vec3 viewPos = worldPos.xyz - vec3(0.0, 0.0, 3.0);

    // Simple perspective projection
    float fov = 1.0;
    float aspect = 800.0 / 600.0;
    float near = 0.1;
    float far = 100.0;

    float f = 1.0 / tan(fov / 2.0);
    mat4 projection = mat4(
        f / aspect, 0.0, 0.0, 0.0,
        0.0, f, 0.0, 0.0,
        0.0, 0.0, (far + near) / (near - far), -1.0,
        0.0, 0.0, (2.0 * far * near) / (near - far), 0.0
    );

    gl_Position = projection * vec4(viewPos, 1.0);

    // Debug color based on normal
    fragColor = inNormal * 0.5 + 0.5;
}
