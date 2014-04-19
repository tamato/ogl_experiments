#version 430

layout(binding=0)  uniform usampler2D BitMask;
layout(location=1) uniform vec2 DepthExtents = vec2(.1,100.);

layout(location=0) out uvec4 FragOut0;
//layout(location=1) out uvec4 FragOut1;

void main() {
    float coord;
    float range = DepthExtents.y - DepthExtents.x;
    float half_range = range * .5;
    float depth = (1./gl_FragCoord.w) - DepthExtents.x;

    if (depth < half_range){
        coord = depth / half_range;
    }else{
        coord = (depth-half_range) / half_range;
    }

    uvec4 mask0 = texture(BitMask, vec2(coord, 0));
    //uvec4 mask1 = texture(BitMask, vec2(coord, 0));

    FragOut0 = mask0;
    //FragOut1 = mask1;
}