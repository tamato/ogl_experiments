#version 430

in vec4 Position;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main() {
    gl_Position = Position;
}
