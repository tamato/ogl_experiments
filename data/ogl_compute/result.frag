#version 430
// It was expressed that some drivers required this next line to function properly
precision highp float;

layout(binding = 0) uniform sampler2D Texture;
out vec4 FragColor;

void main(void) {
    vec2 fragUV = gl_FragCoord.xy / textureSize(Texture, 0).xy;
    vec4 color = texture2D( Texture, fragUV );
    float mag = length(color);
    FragColor = vec4(abs(color.xyz), 1);
}

