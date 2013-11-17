#ifndef OBJLOADER_H
#define OBJLOADER_H

#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace ogle
{
    class ObjLoader
    {
    public:
        enum attribute{
            POSITION,
            NORMALS,
            TEXCOORD0,
            INDEX
        };

        ObjLoader();

        void load(const std::string& filename);

        size_t getIndexCount();
        size_t getVertCount();

        const unsigned int* getIndices();
        const float* getPositions();
        const float* getNormals();

        int getTexCoordLayers();
        const float* getTexCoords(int multiTexCoordLayer);

        size_t getPositionAttributeSize();
        size_t getNormalAttributeSize();
        size_t getIndexAttributeSize();

        size_t getAttributeByteCount(attribute type);

    private:

        std::vector<glm::uvec3> Faces;
        std::vector<glm::vec3> Positions;
        std::vector<glm::vec3> Normals;

        // obj's only have 1 layer ever
        std::vector<glm::vec2> TexCoords;
        unsigned int TexCoordLayers;
    };
}
#endif // OBJLOADER_H
