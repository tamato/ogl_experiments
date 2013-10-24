#version 430

layout(binding = 0, offset = 0) uniform atomic_uint Atomic;
layout(location = 0, index = 0) out vec4 Frag;

void main() {
    atomicCounterIncrement(Atomic);
    Frag = vec4(1,0,0,1);
}