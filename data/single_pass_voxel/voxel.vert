#version 430

layout(location=0) in vec4 Position;

uniform mat4 WorldViewProjection;

void main() {
    gl_Position = WorldViewProjection * Position;
}
