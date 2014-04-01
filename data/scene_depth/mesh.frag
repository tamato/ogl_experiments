#version 430

in vec3 CameraNormal;
//uniform vec3 Diffuse;

layout(location = 0, index = 0) out vec4 Frag;

void main() {
  Frag = vec4(abs(CameraNormal),1);
}