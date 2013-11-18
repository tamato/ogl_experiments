#version 430
#extension GL_ARB_uniform_buffer_object : enable

layout(location = 0) in vec4 Position;
layout(location = 0) uniform mat4 MVP;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main() {
    gl_Position = MVP * Position;

    // from : http://outerra.blogspot.com/2013/07/logarithmic-depth-buffer-optimizations.html
    const float far = 500.0;
    const float Fcoef = 2.0 / log2(far + 1.0);
    float w = gl_Position.w;
    float z = log2(max(1e-6, 1.0 + w)) * Fcoef - 1.0; // newer
    //float z = (2.*log2(w+1.) / log(far+1))-1.;
    gl_Position.z = z;// * w;
}
