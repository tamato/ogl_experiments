#pragma once
#include "common.h"

#include "meshdata.h"

struct DrawableBufferData {
    unsigned int VAO;
    unsigned int VBO;
    unsigned int IBO;
    unsigned int Usage;

    virtual void init(const MeshData& mesh);
    virtual void render(const MeshData& mesh);
    virtual void update(const MeshData& mesh);
    virtual void shutdown();
};

