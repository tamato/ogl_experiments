#version 430

layout(location=0) in vec2 Position;
layout(location=0) out vec2 TexCoords;

void main() {
    gl_Position = vec4(Position,0,1);
    TexCoords = Position.xy * .5 + .5;
}
