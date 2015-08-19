#ifndef CUBE_GENERATOR_H
#define CUBE_GENERATOR_H

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <vector>

/**
    Helper object for making cubes.
    The pivot point of the cube is its center.
    Only creates positions of the verts, if other vertex attributes are needed
    there are seperate utility helpers to generate them.

    this includes index elements
*/
namespace ogle {
    class CubeGenerator
    {
    public:
        CubeGenerator();
        void scale(float scale);
        void tessellation_density(unsigned int stacks, unsigned int slices);
        void tessellation_density(unsigned int density);
        void generate();

    private:
        unsigned int Stacks;
        unsigned int Slices;
        float Scale;

    public:
        std::vector<glm::vec3> Positions;
        std::vector<glm::vec3> Normals;
        std::vector<unsigned int> Indices;
    };
}

#endif // CUBE_GENERATOR_H
