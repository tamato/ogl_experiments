#version 430

layout(location=0) in vec2 TexCoords;

layout(binding=0) uniform usampler2D VolumeTexture0;
layout(binding=1) uniform usampler2D VolumeTexture1;

layout(location=0) out vec4 Frag;

void main() {
    uvec4 z_column = texture(VolumeTexture0, TexCoords);
    uvec4 count = bitCount(z_column);
    uint sum = count.x+count.y+count.z+count.w;

    vec4 color = vec4(0);
    uint test_num = bitCount(0xFF00);
    if (sum == test_num) color = vec4(1);
    else if (sum == 0) color = vec4(0);
    else if (sum == test_num-1) color = vec4(1,1,0,1);
    else if (sum > test_num) color = vec4(1, 0, 0, 1);
    else if (sum < test_num) color = vec4(0, 1, 0, 1);

    color = vec4( float(sum)/128. );
    Frag = color;
}