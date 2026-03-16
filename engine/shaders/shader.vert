#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform PushConstants {
    mat4 viewProjection;
    mat4 model;
} push;

void main() {
    gl_Position = push.viewProjection * push.model * vec4(inPosition, 1.0);

    // Debug color based on normal
    fragColor = inNormal * 0.5 + 0.5;
}
