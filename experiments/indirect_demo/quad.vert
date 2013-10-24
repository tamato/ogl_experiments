#version 430

in vec2 Position;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main() {
    gl_Position = vec4(Position, 0, 1);
}
