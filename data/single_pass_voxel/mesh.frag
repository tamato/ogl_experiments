#version 430

in vec3 fragNormal;
in vec3 fragView;
in vec3 fragToLight;

uniform vec3 Diffuse = vec3(.9);
uniform vec3 LightColor = vec3(1.,1.,.6);

out vec4 Frag;

void main() {
    float light_dist = length(fragToLight);
    float cosTheta = dot(fragNormal, fragToLight/light_dist);
    cosTheta = clamp(cosTheta, 0, 1);
    vec3 color = Diffuse * LightColor * cosTheta;// / (light_dist*light_dist);
    Frag = vec4(color, 1.);
}