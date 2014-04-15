#version 430

layout(location=0) in vec2 TexCoords;
layout(binding=0) uniform usampler2D Texture;
layout(location=0) out uvec4 Frag;

void main() {
    uvec4 value = texture(Texture, TexCoords);
    Frag = value;
}