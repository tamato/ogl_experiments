#version 430

in vec4 Position;
in vec4 Normal;

uniform mat4 WorldViewProjection;
uniform mat4 WorldView;
uniform vec3 LightPos;

out vec3 fragNormal;
out vec3 fragView;
out vec3 fragToLight;

void main() {
    gl_Position = WorldViewProjection * Position;
    fragView = -(WorldView * Position).xyz;
    fragNormal = normalize(WorldView*Normal).xyz; // there is no guarntee that the normal from the .obj is normalized
    fragToLight = (WorldView * vec4(LightPos,1.0)).xyz + fragView;
}
