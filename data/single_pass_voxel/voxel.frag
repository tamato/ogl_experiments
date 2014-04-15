#version 430

layout(binding=0)  uniform usampler1D BitMask;
layout(location=1) uniform vec2 DepthExtents = vec2(1000.0,10000.0);

layout(location=0) out uvec4 FragOut0;
layout(location=1) out uvec4 FragOut1;

void main() {
    float coord = gl_FragCoord.z*.5+.5;
    float range = DepthExtents.y - DepthExtents.x;
    coord = ((1./gl_FragCoord.w) - DepthExtents.x) / range;

    float half_range = range * .5;
    float depth = (1./gl_FragCoord.w) - DepthExtents.x;
    if (depth < half_range){
        coord = depth / half_range;
        uvec4 mask = texture(BitMask, coord);
        FragOut0 = mask;
    }
    else{
        coord = (depth-half_range) / half_range;
        uvec4 mask = texture(BitMask, coord);
        FragOut1 = mask;
    }
}