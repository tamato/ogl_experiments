#version 430

in vec4 fragPosition;
uniform sampler1D BitMask;
out vec4 Frag[2];

void main() {
    vec2 coord = vec2(fragPosition.z, 0.);
    float d = texture(BitMask, coord.x).r;
    Frag[0] = vec4(vec3(d), 1.);
    Frag[1] = vec4(vec3(d), 1.);
}