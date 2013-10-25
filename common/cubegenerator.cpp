#include "cubegenerator.h"

using namespace ogle;
using namespace std;

CubeGenerator::CubeGenerator()
    : Stacks(0)
    , Slices(0)
    , Rows(0)
    , Scale(0)
{

}

void CubeGenerator::scale(float scale)
{
    Scale = scale;
}

void CubeGenerator::tessellation_density(unsigned int stacks, unsigned int slices, unsigned int rows)
{
    Stacks = stacks;
    Slices = slices;
    Rows = rows;
}

void CubeGenerator::tessellation_density(unsigned int density)
{
    Stacks = Slices = Rows = density;
}

void CubeGenerator::generate()
{
    Positions.clear();
    Positions.resize(Stacks*Slices*Rows);

    for (unsigned int i=0; i<Stacks; ++i) {
        for (unsigned int j=0; j<Slices; ++j) {

        }
    }
}
