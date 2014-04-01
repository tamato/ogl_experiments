/**
*   Test out the math to do thin wire aa
*   Based off of :
*   http://www.humus.name/index.php?page=3D&ID=89
*/

#include <string>
#include <iostream>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_CXX11
// #define GLM_MESSAGES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

int main(int argc, char *argv[])
{
    float fovy_radians = 1.0f;
    glm::mat4 Projection = glm::perspective(fovy_radians, 1.0f, 1.0f, 100.0f);
    glm::mat4 View = glm::lookAt(
        glm::vec3(0,0,1),
        glm::vec3(0,0,0),
        glm::vec3(0,1,0)
    );

    float window_height = 1024.f;
    float pixel_scale = tanf(fovy_radians*0.5f) / window_height;

    glm::mat4 view_proj = Projection*glm::inverse(View);
    std::cout << "Projection: " << glm::to_string(Projection[3]) << std::endl;
    std::cout << "ViewProj: " << glm::to_string(view_proj[3]) << std::endl;

    std::cout << "The question trying to be answered here, is why does Hummus dot viewProj[3] with position?" << std::endl;
    std::cout << "Test what the dot is of the center point at the near plane, back to the far plane" << std::endl;
    glm::vec4 view_proj3 = view_proj[3];
    glm::vec4 trans_view_proj3 = glm::transpose(view_proj)[3];
    for (float z=0.0f;z<100.0f;z+=1.0f){

        std::cout << "At depth: " << z << "\n";
        glm::vec4 pos(0,0,z,1);

        float projected_depth = glm::dot(pos, view_proj3);
        std::cout << "\tView_proj[3] . pos = depth equals: " << projected_depth << "\n";

        float trans_projected_depth = glm::dot(pos, trans_view_proj3);
        std::cout << "\tTransposed_view_proj[3] . pos = depth equals: " << trans_projected_depth << "\n";

        glm::vec4 test = view_proj * pos;
        std::cout << "\tView_proj * pos = depth equals: " << test[3] << "\n";
        std::cout << "\tPixel radius at this depth: " << pixel_scale * -test[3] << std::endl;
    }

    std::cout   << std::endl;
    std::cout   << "Tests show that for column-major matrices,\n"
                << "the view_proj needs to be transposed first in order to use just a dot product to find projected depth."
                << std::endl;

    return 0;
}