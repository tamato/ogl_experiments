#include <iostream>

#include "drawablebufferdata.h"


void DrawableBufferData::init(const MeshData& mesh)
{
    unsigned int buffs[2];
    glGenBuffers(2, buffs);
    VBO = buffs[0];
    IBO = buffs[1];

    glGenVertexArrays(1, &VAO);

    Usage = GL_DYNAMIC_DRAW;

    glBindVertexArray(VAO);
    glEnableVertexAttribArray(0);

    const size_t points_per_quad = 6;
    const size_t bytes_per_particle = sizeof(glm::vec3) * points_per_quad;

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, bytes_per_particle, nullptr, Usage);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
}

void DrawableBufferData::render(const MeshData& mesh)
{
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    
    const size_t points_per_quad = 6;
    const size_t bytes_per_particle = sizeof(glm::vec3) * points_per_quad;
    const size_t particle_count = mesh.VertByteCount / bytes_per_particle;
    
    for (size_t i=0; i<particle_count; ++i){
        const size_t start_idx = bytes_per_particle * i;
        glBufferData(GL_ARRAY_BUFFER, bytes_per_particle, &mesh.VertData[start_idx], Usage); 
        glDrawArrays(GL_TRIANGLES, 0, bytes_per_particle);
    }
}

void DrawableBufferData::update(const MeshData& mesh)
{
}

void DrawableBufferData::shutdown()
{
    unsigned int buffs[2] = { VBO, IBO };
    glDeleteBuffers(2, buffs);
    glDeleteVertexArrays(1, &VAO);
}
