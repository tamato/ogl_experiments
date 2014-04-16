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
    if (sum == 0) color = vec4(0);
    else if (sum < 31)
        color = vec4(1,0,0,1);
    else if (sum < 63)
        color = vec4(0,1,0,1);
    else if (sum < 95)
        color = vec4(0,0,1,1);
    else if (sum < 127)
        color = vec4(1,1,1,1);
    else
        color = vec4(1,0,1,1);

    //color = vec4( float(abs(sum))/128. );
    Frag = color;
}