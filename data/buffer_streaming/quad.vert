#version 430

layout(location = 0) in vec3 Position;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main() {
    gl_Position = vec4(Position, 1);
}
