#version 430

in vec4 Position;
in vec4 Normal;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main() {
    gl_Position = Position;
}
