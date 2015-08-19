#version 430
// It was expressed that some drivers required this next line to function properly
precision highp float;

layout(binding = 0) uniform sampler2D Texture;
out vec4 FragColor;

void main(void) {
    vec2 fragUV = gl_FragCoord.xy / textureSize(Texture, 0).xy;
    vec4 color = texture2D( Texture, fragUV );
    bvec3 b = lessThan(color.xyz, vec3(0));
    if (b.x)
        color.xyz = vec3(-color.x);
    if (b.y)
        color.b = -color.y;
    FragColor = vec4(color.xyz * 100., 1);
}

