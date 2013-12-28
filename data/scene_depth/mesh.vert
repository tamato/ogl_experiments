#version 430
#extension GL_ARB_uniform_buffer_object : enable

layout(location = 0) in vec4 Position;
layout(location = 1) in vec3 Normal;
layout(location = 0) uniform mat4 MVP;

out gl_PerVertex
{
    vec4 gl_Position;
};

out vec3 CameraNormal;

void main() {
    gl_Position = MVP * Position;
    CameraNormal = Normal;
}
