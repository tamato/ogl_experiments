#version 430

layout(location=0) in vec4 Position;
layout(location=1) in vec3 Normal;

layout(location=0) uniform mat4 WorldViewProjection;
layout(location=1) uniform mat4 WorldView;
layout(location=2) uniform vec3 LightPos;

layout(location=0) out vec3 fragNormal;
layout(location=1) out vec3 fragView;
layout(location=2) out vec3 fragToLight;

void main() {
    gl_Position = WorldViewProjection * Position;
    vec3 camera_space_pos = (WorldView * Position).xyz;
    fragView = -camera_space_pos;
    fragNormal = mat3(WorldView) * Normal;
    fragToLight = (WorldView * vec4(LightPos,1.0)).xyz - camera_space_pos;
}
