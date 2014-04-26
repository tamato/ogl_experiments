#version 430

layout(location=0) in vec3 fragNormal;
layout(location=1) in vec3 fragView;
layout(location=2) in vec3 fragToLight;


layout(binding=0) uniform usampler2D Density0;
layout(binding=1) uniform usampler2D Density1;
layout(binding=2) uniform usampler2D BitMask;
layout(binding=3) uniform usampler2D ColumnBitMask;

layout(location=3) uniform vec2 DepthExtents = vec2(.1,100.);

layout(location=0) out vec4 Frag;

void main() {
    // the density textures are the same size, just grab from one of them
    vec2 texture_size = textureSize(Density0, 0);
    vec2 its = 1./ texture_size; // inverse_texture_size
    vec2 density_coord = gl_FragCoord.xy / texture_size;
    uvec4 curr0 = texture(Density0, density_coord + vec2( 0, 0)*its);
    uvec4 posx0 = texture(Density0, density_coord + vec2( 1, 0)*its);
    uvec4 negx0 = texture(Density0, density_coord + vec2(-1, 0)*its);
    uvec4 posy0 = texture(Density0, density_coord + vec2( 0, 1)*its);
    uvec4 negy0 = texture(Density0, density_coord + vec2( 0,-1)*its);

    uvec4 curr1 = texture(Density1, density_coord + vec2( 0, 0)*its);
    uvec4 posx1 = texture(Density1, density_coord + vec2( 1, 0)*its);
    uvec4 negx1 = texture(Density1, density_coord + vec2(-1, 0)*its);
    uvec4 posy1 = texture(Density1, density_coord + vec2( 0, 1)*its);
    uvec4 negy1 = texture(Density1, density_coord + vec2( 0,-1)*its);

    // figure out which, or both, density to sample from
    float range = DepthExtents.y - DepthExtents.x;
    float half_range = range * .5;
    float depth = (1./gl_FragCoord.w) - DepthExtents.x;

    vec2 coord;
    if (depth < half_range){
        coord = vec2(depth / half_range);
    }else{
        coord = vec2((depth-half_range) / half_range);
    }

    // grab the info to make our normal


    const vec3 Diffuse = vec3(.9);
    const vec3 Ambient = vec3(0.1,0.1,0.2);
    const vec3 LightColor = vec3(1.,1.,.6);
    const float LightPower = 500000.0;

    float light_dist = length(fragToLight);
    float cosTheta = dot(fragNormal, fragToLight/light_dist);
    cosTheta = clamp(cosTheta, 0, 1);
    vec3 color = Ambient + Diffuse * LightColor * LightPower * cosTheta / (light_dist*light_dist);
    Frag = vec4(color, 1.);
}