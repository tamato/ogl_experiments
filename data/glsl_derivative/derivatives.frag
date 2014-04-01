#version 430

in vec3 fragPosition;
layout(location = 0, index = 0) out vec4 Frag;

void main() {

    vec3 p0 = dFdx(fragPosition);
    vec3 p1 = dFdy(fragPosition);
    vec3 N  = cross(p1,p0);

    float perpTest = dot(p0,p1);
    vec3 color = vec3(0.);
    if (perpTest < 0.0000001){
       color.r = 1.0;
    }

    vec3 T = cross(N, p1);
    vec3 B = cross(p0, N);

    perpTest = dot(T,B);
    if (perpTest < 0.00000000001){
       color.b = 1.0;
    }

    color = max( vec3(0.), normalize(N) + T + B);
    Frag = vec4(color,1.);
}

