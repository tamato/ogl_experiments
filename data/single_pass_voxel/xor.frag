#version 430
#extension GL_EXT_gpu_shader4 : enable

uniform usampler2DRect Sampler;
out uvec4 Frag;

void main() {
    Frag = texture2DRect(Sampler, gl_FragCoord.xy);
}