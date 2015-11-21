#include "buffersubdata_post_orphan.h"

BufferSubDataPostOrphan::BufferSubDataPostOrphan()
: DrawableBufferData()
{

}

BufferSubDataPostOrphan::~BufferSubDataPostOrphan()
{

}

void BufferSubDataPostOrphan::render(const MeshData& mesh)
{
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    
    const size_t points_per_quad = 6;
    const size_t bytes_per_particle = sizeof(glm::vec3) * points_per_quad;
    const size_t particle_count = mesh.VertByteCount / bytes_per_particle;
    
    for (size_t i=0; i<particle_count; ++i){
        const size_t start_idx = bytes_per_particle * i;
        glBufferSubData(GL_ARRAY_BUFFER, 0, bytes_per_particle, &mesh.VertData[start_idx]);
        glDrawArrays(GL_TRIANGLES, 0, bytes_per_particle);
        glBufferData(GL_ARRAY_BUFFER, bytes_per_particle, nullptr, Usage);
    }
}
