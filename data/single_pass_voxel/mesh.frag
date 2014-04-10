#version 430

layout(location=0) in vec3 fragNormal;
layout(location=1) in vec3 fragView;
layout(location=2) in vec3 fragToLight;

uniform vec3 Diffuse = vec3(.9);
uniform vec3 Ambient = vec3(0.1,0.1,0.2);
uniform vec3 LightColor = vec3(1.,1.,.6);
uniform float LightPower = 500000.0;

out vec4 Frag;

void main() {
    float light_dist = length(fragToLight);
    float cosTheta = dot(fragNormal, fragToLight/light_dist);
    cosTheta = clamp(cosTheta, 0, 1);
    vec3 color = Ambient + Diffuse * LightColor * LightPower * cosTheta / (light_dist*light_dist);
    Frag = vec4(color, 1.);
}