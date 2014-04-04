#version 430

in vec4 Position;

uniform mat4 MVP;

void main() {
    gl_Position = MVP * Position;
}
