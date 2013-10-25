#ifndef CUBE_GENERATOR_H
#define CUBE_GENERATOR_H

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
        void tessellation_density(unsigned int stacks, unsigned int slices, unsigned int rows);
        void tessellation_density(unsigned int density);
        void generate();

    private:
        unsigned int Stacks;
        unsigned int Slices;
        unsigned int Rows;
        float Scale;
        std::vector<glm::vec3> Positions;
    };
}

#endif // CUBE_GENERATOR_H