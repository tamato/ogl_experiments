/**
    Demo to compare a traditional depth buffer to a logrithmic one
    http://outerra.blogspot.com/2009/08/logarithmic-z-buffer.html
*/

#include <iostream>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_CXX11
// #define GLM_MESSAGES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

using namespace std;

namespace
{
    int Bits = 24;
    float Far = 100.0f;
    float Near = 1.0f;
    float C = 1.0f;
    glm::mat4 Projection = glm::perspective(45.0f, 4.0f / 3.0f, Near, Far);
}

float log_depth(const glm::vec4& pt)
{
    return log(C*pt.w + 1.0f) / log(C*Far + 1.0);
}

float float_resolution(const glm::vec4& pt)
{
    return log(C*Far + 1.0f) / ( float(pow(2, Bits) - 1) * C / (C*pt.w + 1.0f));
}

void test(const glm::vec4& pt )
{
    glm::vec4 clip_pt = Projection * pt;
    float normal_z = clip_pt.z / clip_pt.w;

    cout.precision(2);
    cout.setf(ios::fixed);
    cout << "Clipspace z: " << clip_pt.z
         << "\tNormalDepth " << normal_z
         << "\tLogDetph " << log_depth(clip_pt);
    cout.precision(10);
    // Float point resolution is how much a number needs to change by inorder to be noticable
    cout << "\tFloating point resolution " << float_resolution(clip_pt) << endl;
}

int main(int argc, char *argv[])
{
    cout << "Near " << Near << " Far " << Far << endl;
    for (float i=Near; i<Far; i+=1)
        test(glm::vec4(0,0,-1.0 * i,1));
    exit( EXIT_SUCCESS );
}