#version 430

layout(binding=0) uniform sampler1D BitMask;
layout(location=0) out vec4 FragOut;

void main() {
    float coord = gl_FragCoord.z*.5+.5;
    float d = texture(BitMask, coord).r;
    FragOut = vec4(vec3(d), 1.);
}