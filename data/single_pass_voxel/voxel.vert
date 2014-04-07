#version 430

in vec4 Position;

uniform mat4 WorldViewProjection;

out vec4 fragPosition;

void main() {
    gl_Position = WorldViewProjection * Position;
    fragPosition = gl_Position;
}
