#include <iostream>

#include "map_persistent.h"


void MapPersistent::init(const MeshData& mesh)
{
    BufferNumber = 0;
    BufferCount = 2;

    glGenBuffers(1, &VBO);
    glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);
    glEnableVertexAttribArray(0);

    Bytes = mesh.VertByteCount * BufferCount;
    const GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBufferStorage(GL_ARRAY_BUFFER, Bytes, nullptr, flags /*| GL_CLIENT_STORAGE_BIT*/);
    MappedVertPointer = glMapBufferRange(GL_ARRAY_BUFFER, 0, Bytes, flags);

    for (int i=0;i<BufferCount; ++i){
        const int start_idx = i * mesh.VertByteCount;
        void* dst = (unsigned char*)MappedVertPointer + start_idx;
        memcpy(dst, (void*)mesh.VertData, mesh.VertByteCount);
    }
}

void MapPersistent::render(const MeshData& mesh)
{
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    const size_t points_per_quad = 6;
    const size_t bytes_per_quad = sizeof(glm::vec3) * points_per_quad;
    const size_t particle_count = mesh.VertByteCount / bytes_per_quad;

    for (size_t p=0; p<particle_count; ++p){

        // update the buffer
        const size_t particle_byte_idx = p * bytes_per_quad;
        const size_t start_byte_idx = (BufferNumber * mesh.VertByteCount) + particle_byte_idx;
        void* dst = ((unsigned char*)MappedVertPointer) + start_byte_idx;
        memcpy(dst, (void*)&mesh.VertData[particle_byte_idx], bytes_per_quad);

        // draw it
        const size_t particle_idx = p * points_per_quad;
        const size_t start_idx = (BufferNumber * points_per_quad * particle_count) + particle_idx;
        glDrawArrays(GL_TRIANGLES, start_idx, points_per_quad);
    }

    BufferNumber = (BufferNumber+1) % BufferCount;
}

void MapPersistent::update(const MeshData& mesh)
{
    // const int start_idx = BufferNumber * mesh.VertByteCount;
    // void* dst = ((unsigned char*)MappedVertPointer) + start_idx;
    // memcpy(dst, mesh.VertData, mesh.VertByteCount);
}

void MapPersistent::shutdown()
{
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glDisableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
}
