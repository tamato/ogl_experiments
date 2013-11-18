#version 430

layout(location = 0, index = 0) out vec4 Frag;

void main() {
    float depth = gl_FragCoord.z;
    //Frag = vec4(depth*.5+.5);
    Frag = vec4(depth);
}