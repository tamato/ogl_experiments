#version 430

in vec4 Position;
in vec3 Normal;

uniform mat4 WorldViewProjection;
uniform mat4 WorldView;
uniform vec3 LightPos;

out vec3 fragNormal;
out vec3 fragView;
out vec3 fragToLight;

void main() {
    gl_Position = WorldViewProjection * Position;
    vec3 camera_space_pos = (WorldView * Position).xyz;
    fragView = -camera_space_pos;
    fragNormal = mat3(WorldView) * Normal;
    fragToLight = (WorldView * vec4(LightPos,1.0)).xyz - camera_space_pos;
}
