#version 430

layout(location=0) in vec2 TexCoords;

layout(binding=0) uniform usampler2D Density0;
layout(binding=1) uniform usampler2D Density1;

layout(location=0) out vec4 Frag;

void main() {
    vec2 its = 1./ textureSize(Density0, 0); // inverse_texture_size
    uvec4 curr = texture(Density0, TexCoords + vec2( 0, 0)*its);
    uvec4 posx = texture(Density0, TexCoords + vec2( 1, 0)*its);
    uvec4 negx = texture(Density0, TexCoords + vec2(-1, 0)*its);
    uvec4 posy = texture(Density0, TexCoords + vec2( 0, 1)*its);
    uvec4 negy = texture(Density0, TexCoords + vec2( 0,-1)*its);



    Frag = vec4( curr );
}