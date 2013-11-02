#version 430

layout(binding = 0, offset = 4) uniform atomic_uint Atomic;

void main() {
    atomicCounterIncrement(Atomic);
}