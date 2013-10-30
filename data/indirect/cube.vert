#version 430

layout(location = 0) in vec4 Position;
layout(location = 1) in vec4 Normal;
layout(binding = 0, offset = 0) uniform atomic_uint Atomic;

// using std140 to match sure the stride with the mat4
// is something sane.
// see the last section of the following to see what I mean.
// http://www.opengl.org/wiki/Uniform_Buffer_Object#Layout_queries
layout(std140) uniform transform
{
    mat4 MVP;
    mat4 Normal;
} Transform;

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(location = 2) out vec4 normal;

void main() {
    gl_Position = Transform.MVP * Position;
    normal = Normal;

    atomicCounterIncrement(Atomic);
}
