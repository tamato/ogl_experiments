#version 430

layout(location = 2) in vec3 normal;
layout(location = 0, index = 0) out vec4 Frag;

void main() {
    Frag = vec4(1,0,0,1);
    Frag = vec4(abs(normal), 1);
}