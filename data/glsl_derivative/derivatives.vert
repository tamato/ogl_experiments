#version 430
#extension GL_ARB_uniform_buffer_object : enable

layout(location = 0) in vec4 Position;
layout(location = 0) uniform mat4 Transforms;

out vec3 fragPosition;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main() {
    gl_Position = Transforms * Position;
    fragPosition = gl_Position.xyz;
}
