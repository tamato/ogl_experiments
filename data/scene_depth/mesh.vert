#version 430
#extension GL_ARB_uniform_buffer_object : enable

layout(location = 0) in vec4 Position;
layout(location = 0) uniform mat4 MVP;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main() {
    gl_Position = MVP * Position;
}
