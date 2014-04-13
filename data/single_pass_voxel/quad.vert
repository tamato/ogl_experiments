#version 430

layout(location=0) in vec4 Position;

layout(location=0) out vec2 TexCoords;

void main() {
    gl_Position = Position;
    TexCoords = Position.xy * .5 + .5;
}
