#include <iostream>

#include "tripplevbo_no_orphan.h"


void TrippleVBONoOrphan::init(const MeshData& mesh)
{
    const unsigned int start_count = 2;
    BufferObjects.Syncs.resize(start_count);
    BufferObjects.VBO_Available.resize(start_count, true);

    BufferObjects.VAOs.resize(start_count);
    BufferObjects.VBOs.resize(start_count);
    ActiveIndex = 0;

    glGenBuffers(start_count, BufferObjects.VBOs.data());
    glGenVertexArrays(start_count, BufferObjects.VAOs.data());

    Usage = GL_STREAM_DRAW;

    for (int i=0; i<start_count; ++i){
        glBindVertexArray(BufferObjects.VAOs[i]);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, BufferObjects.VBOs[i]);
        glBufferData(GL_ARRAY_BUFFER, mesh.VertByteCount, mesh.VertData, Usage);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    }
}

void TrippleVBONoOrphan::render(const MeshData& mesh)
{
    glBindVertexArray(BufferObjects.VAOs[ActiveIndex]);
    glDrawArrays(GL_TRIANGLES, 0, mesh.VertByteCount);
    BufferObjects.Syncs[ActiveIndex] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    BufferObjects.VBO_Available[ActiveIndex] = false;
}

void TrippleVBONoOrphan::update(const MeshData& mesh)
{
    int readiedSync = -1;
    for (unsigned int i=0; i<BufferObjects.Syncs.size(); ++i){
        if (BufferObjects.VBO_Available[i] == false){   // check if it is ready yet
            GLenum signaled_state = glClientWaitSync(BufferObjects.Syncs[i], GL_SYNC_FLUSH_COMMANDS_BIT, 0);

            if (signaled_state == GL_ALREADY_SIGNALED || signaled_state == GL_CONDITION_SATISFIED){
                glDeleteSync(BufferObjects.Syncs[i]);
                BufferObjects.VBO_Available[i] = true;
                readiedSync = i;
                break;
            }
        }
        else if (BufferObjects.VBO_Available[i] == true){
            readiedSync = i;
            break;
        }
    }

    if (readiedSync == -1){
        std::cerr << "No syncs/vbos are ready for update, creating new one." << std::endl;
        
        ActiveIndex = BufferObjects.Syncs.size();
        size_t size = ActiveIndex + 1;
        BufferObjects.Syncs.resize(size);
        BufferObjects.VBO_Available.resize(size);
        BufferObjects.VAOs.resize(size);
        BufferObjects.VBOs.resize(size);
        BufferObjects.VBO_Available[ActiveIndex] = true;
        
        glGenBuffers(1, &BufferObjects.VBOs[ActiveIndex]);
        glGenVertexArrays(1, &BufferObjects.VAOs[ActiveIndex]);
        
        glBindVertexArray(BufferObjects.VAOs[ActiveIndex]);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, BufferObjects.VBOs[ActiveIndex]);
        glBufferData(GL_ARRAY_BUFFER, mesh.VertByteCount, nullptr, Usage);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    }
    else {
        ActiveIndex = readiedSync;
    }

    glBindBuffer(GL_ARRAY_BUFFER, BufferObjects.VBOs[ActiveIndex]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, mesh.VertByteCount, mesh.VertData);
}

void TrippleVBONoOrphan::shutdown()
{
    glDeleteBuffers(BufferObjects.VBOs.size(), BufferObjects.VBOs.data());
    glDeleteVertexArrays(BufferObjects.VAOs.size(), BufferObjects.VAOs.data());
}
