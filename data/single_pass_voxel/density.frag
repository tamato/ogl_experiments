#version 430

layout(binding=0)  uniform usampler2D VoxelGrid;

layout(location=0) out uvec4 FragOut0;
layout(location=1) out uvec4 FragOut1;


uvec4 pairwise_sum(in uvec4 texel){
    const uvec4 z_odd_mask  = uvec4(0xAAAAAAAA);    // bit pattern: 1010 1010
    const uvec4 z_even_mask = uvec4(0x55555555);    // bit pattern: 0101 0101
    return (texel & z_even_mask) + ((texel & z_odd_mask)>>1u);
}

uvec4 even_sum(in uvec4 pair_sum){
    const uvec4 xy_even_mask = uvec4(0x33333333);   // bit pattern: 0011 0011
    return pair_sum & xy_even_mask;
}

uvec4 odd_sum(in uvec4 pair_sum){
    const uvec4 xy_odd_mask  = uvec4(0xCCCCCCCC);   // bit pattern: 1100 1100
    return (pair_sum & xy_odd_mask) >> 2u;
}

// this function is from:
// http://graphics.stanford.edu/~seander/bithacks.html#InterleaveBMN
uvec4 interleave_nibbles(in uvec4 even, in uvec4 odd){
    const uvec4 B[] = {uvec4(0x0F0F0F0F), uvec4(0x00FF00FF)};
    const uvec4 S[] = {uvec4(4), uvec4(8)};

    even = (even | (even << S[1])) & B[1];
    even = (even | (even << S[0])) & B[0];

    odd = (odd | (odd << S[1])) & B[1];
    odd = (odd | (odd << S[0])) & B[0];

    uvec4 r = even | (odd << 4u);
    return r;
}

void main() {
    ivec2 coord = ivec2(gl_FragCoord.xy)*2;

    uvec4 current     = texelFetch(VoxelGrid, coord+ivec2(0,0), 0);
    uvec4 x_neighbor  = texelFetch(VoxelGrid, coord+ivec2(1,0), 0);
    uvec4 y_neighbor  = texelFetch(VoxelGrid, coord+ivec2(0,1), 0);
    uvec4 xy_neighbor = texelFetch(VoxelGrid, coord+ivec2(1,1), 0);

    uvec4 z_pairwise_sum0 = pairwise_sum(current);
    uvec4 z_pairwise_sum1 = pairwise_sum(x_neighbor);
    uvec4 z_pairwise_sum2 = pairwise_sum(y_neighbor);
    uvec4 z_pairwise_sum3 = pairwise_sum(xy_neighbor);

    uvec4 even_sumed = uvec4(0);
    even_sumed += even_sum(z_pairwise_sum0);
    even_sumed += even_sum(z_pairwise_sum1);
    even_sumed += even_sum(z_pairwise_sum2);
    even_sumed += even_sum(z_pairwise_sum3);

    uvec4 odd_sumed = uvec4(0);
    odd_sumed += odd_sum(z_pairwise_sum0);
    odd_sumed += odd_sum(z_pairwise_sum1);
    odd_sumed += odd_sum(z_pairwise_sum2);
    odd_sumed += odd_sum(z_pairwise_sum3);

    FragOut0 = interleave_nibbles(even_sumed>>16u, odd_sumed>>16u);
    FragOut1 = interleave_nibbles(even_sumed&0xFFFF, odd_sumed&0xFFFF);
}