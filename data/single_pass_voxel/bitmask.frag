#version 430

layout(location=0) in vec2 TexCoords;
layout(binding=0)  uniform usampler2D BitMask;

layout(location=0) out uvec4 FragOut;

void main() {
    uvec4 mask = texture(BitMask, TexCoords);
    FragOut = mask;
}