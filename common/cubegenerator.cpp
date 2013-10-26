#include "cubegenerator.h"

using namespace ogle;
using namespace std;

CubeGenerator::CubeGenerator()
    : Stacks(2)
    , Slices(2)
    , Scale(1)
{

}

void CubeGenerator::scale(float scale)
{
    Scale = scale;
}

void CubeGenerator::tessellation_density(unsigned int stacks, unsigned int slices)
{
    if (stacks < 2) stacks = 2;
    if (slices < 2) slices = 2;
    Stacks = stacks;
    Slices = slices;
}

void CubeGenerator::tessellation_density(unsigned int density)
{
    if (density < 2) density = 2;
    Stacks = Slices = density;
}

void CubeGenerator::generate()
{
    Positions.clear();
    Positions.resize(Stacks*Slices*6); // 6  == sides to a cube

    Normals.clear();
    Normals.resize(Stacks*Slices*6);

    float scale = Scale * 2.0f;

    // generate a plane that the verts will be based off of
    vector<glm::vec2> plane(Stacks*Slices);

    // z faces
    unsigned int offset0 = 0;
    unsigned int offset1 = Stacks*Slices;
    for (unsigned int v=0; v<Stacks; ++v) {
        for (unsigned int u=0; u<Slices; ++u) {
            float x = scale * (u / float(Slices) - 0.5f);
            float y = scale * (v / float(Stacks) - 0.5f);
            plane[u + v*Slices] = glm::vec2(x,y);

            float z = scale * 1;
            Positions[offset0 + u + v*Slices] = glm::vec3(x, y, z);
            Normals  [offset0 + u + v*Slices] = glm::vec3(0, 0, 1);
            Positions[offset1 + u + v*Slices] = glm::vec3(x, y,-z);
            Normals  [offset1 + u + v*Slices] = glm::vec3(0, 0,-1);
        }
    }

    // x faces
    offset0 += Stacks*Slices;
    offset1 += Stacks*Slices;
    for (unsigned int v=0; v<Stacks; ++v) {
        for (unsigned int u=0; u<Slices; ++u) {
            float x = scale * 1;
            float y = plane[u + v*Slices].y;
            float z = plane[u + v*Slices].x;
            Positions[offset0 + u + v*Slices] = glm::vec3( x, y, z);
            Normals  [offset0 + u + v*Slices] = glm::vec3( 1, 0, 0);
            Positions[offset1 + u + v*Slices] = glm::vec3(-x, y, z);
            Normals  [offset1 + u + v*Slices] = glm::vec3(-1, 0, 0);
        }
    }

    // y faces
    offset0 += Stacks*Slices;
    offset1 += Stacks*Slices;
    for (unsigned int v=0; v<Stacks; ++v) {
        for (unsigned int u=0; u<Slices; ++u) {
            float x = plane[u + v*Slices].x;
            float y = scale * 1;
            float z = plane[u + v*Slices].y;
            Positions[offset0 + u + v*Slices] = glm::vec3(x, y, z);
            Normals  [offset0 + u + v*Slices] = glm::vec3(0, 1, 0);
            Positions[offset1 + u + v*Slices] = glm::vec3(x,-y, z);
            Normals  [offset1 + u + v*Slices] = glm::vec3(0, 1, 0);
        }
    }

    // 6 faces to a cube, 2 triangles each face, 3 verts per triangle
    Indices.clear();
    Indices.resize(Stacks*Slices*6*2*3);
    unsigned int counter = 0;
    for (unsigned int k=0; k<6; ++k){
        for (unsigned int j=0; j<Stacks; ++j){
            for (unsigned int i=0; i<Slices-1; ++i) {
                Indices[counter + 0] = counter+0;
                Indices[counter + 1] = counter+1;
                Indices[counter + 2] = counter+Slices;

                Indices[counter + 3] = counter+1;
                Indices[counter + 4] = counter+Slices+1;
                Indices[counter + 5] = counter+Slices;
                counter += 6;
            }
        }
    }

    // x^6 + y^6 + z^6 = a^6
    // x(u, v) = (a sin(u) cos(v))/(sin^6(u) (sin^6(v)+cos^6(v))+cos^6(u))^(1/6)
    // y(u, v) = (a sin(u) sin(v))/(sin^6(u) (sin^6(v)+cos^6(v))+cos^6(u))^(1/6)
    // z(u, v) = (a cos(u))/(sin^6(u) (sin^6(v)+cos^6(v))+cos^6(u))^(1/6)
}
