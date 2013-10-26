#version 430

layout(binding = 0, offset = 0) uniform atomic_uint Atomic;

void main() {
    atomicCounterIncrement(Atomic);
}