#version 430

out vec4 Frag;

layout(binding = 1, offset = 0) uniform atomic_uint Atomic2;
layout(std140) uniform atomic
{
    uint AtomicMax;
};

uniform float Current = 0.0;
uniform float Tolerance = 0.0;

void main() {
    uint val = atomicCounterIncrement(Atomic2);
    float p = float(val) / float(AtomicMax);
    vec3 color = vec3(1,p,0);

    if ( p < (Current-Tolerance) )
        color = vec3(0);
    else if (p > (Current-Tolerance) && p < (Current+Tolerance) )
        color = vec3(1);

    Frag = vec4(color, 1);
}