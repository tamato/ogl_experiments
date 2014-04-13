#version 430

layout(location=0) uniform vec2 DepthExtents = vec2(1000.0,10000.0);
layout(binding=0) uniform sampler1D BitMask;
layout(location=0) out vec4 FragOut;

void main() {
    float coord = gl_FragCoord.z*.5+.5;
    //coord = (-gl_FragCoord.w - DepthExtents.x) / (DepthExtents.y - DepthExtents.x);
    vec4 mask = texture(BitMask, coord);
    FragOut = mask;
}