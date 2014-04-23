#version 430

layout(binding=0)  uniform usampler2D VoxelGrid;

layout(location=0) out uvec4 FragOut0;
layout(location=1) out uvec4 FragOut1;

const uvec4 z_odd_mask  = uvec4(0xAAAAAAAA);    // bit pattern: 1010 1010
const uvec4 z_even_mask = uvec4(0x55555555);    // bit pattern: 0101 0101

const uvec4 xy_odd_mask  = uvec4(0xCCCCCCCC);   // bit pattern: 1100 1100
const uvec4 xy_even_mask = uvec4(0x33333333);   // bit pattern: 0011 0011

void main() {
    ivec2 coord = ivec2(gl_FragCoord.xy)*2;

    uvec4 current     = texelFetch(VoxelGrid, coord+ivec2(0,0), 0);
    uvec4 x_neighbor  = texelFetch(VoxelGrid, coord+ivec2(1,0), 0);
    uvec4 y_neighbor  = texelFetch(VoxelGrid, coord+ivec2(0,1), 0);
    uvec4 xy_neighbor = texelFetch(VoxelGrid, coord+ivec2(1,1), 0);

    uvec4 z_pairwise_sum0 = (current & z_even_mask)     + ((current & z_odd_mask)>>1u);
    uvec4 z_pairwise_sum1 = (x_neighbor & z_even_mask)  + ((x_neighbor & z_odd_mask)>>1u);
    uvec4 z_pairwise_sum2 = (y_neighbor & z_even_mask)  + ((y_neighbor & z_odd_mask)>>1u);
    uvec4 z_pairwise_sum3 = (xy_neighbor & z_even_mask) + ((xy_neighbor & z_odd_mask)>>1u);

    uvec4 even_sum = uvec4(0);
    even_sum += (z_pairwise_sum0 & xy_even_mask);
    even_sum += (z_pairwise_sum1 & xy_even_mask);
    even_sum += (z_pairwise_sum2 & xy_even_mask);
    even_sum += (z_pairwise_sum3 & xy_even_mask);

    uvec4 odd_sum = uvec4(0);
    odd_sum += (z_pairwise_sum0 & xy_odd_mask) >> 2u;
    odd_sum += (z_pairwise_sum1 & xy_odd_mask) >> 2u;
    odd_sum += (z_pairwise_sum2 & xy_odd_mask) >> 2u;
    odd_sum += (z_pairwise_sum3 & xy_odd_mask) >> 2u;

    FragOut0 = uvec4(odd_sum.rg, even_sum.rg);
    FragOut1 = uvec4(even_sum);
}