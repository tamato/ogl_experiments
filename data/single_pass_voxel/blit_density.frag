#version 430

layout(location=0) in vec2 TexCoords;

layout(binding=0) uniform usampler2D DensityTexture0;
layout(binding=1) uniform usampler2D DensityTexture1;

layout(location=0) out vec4 Frag;

const float MaxBitCount = float(bitCount(0xFFFFFFFF)*2);

void main() {
    uvec4 z_column0 = texture(DensityTexture0, TexCoords);
    uvec4 z_column1 = texture(DensityTexture1, TexCoords);

    uvec4 count0 = bitCount(z_column0);
    uvec4 count1 = bitCount(z_column1);

    uint sum = count0.x+count0.y+count0.z+count0.w;
    sum += count1.x+count1.y+count1.z+count1.w;

    Frag = vec4( float(sum)/MaxBitCount );
}