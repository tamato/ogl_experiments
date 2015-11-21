#pragma once

#include <vector>

#include "common.h"
#include "meshdata.h"

#include "drawablebufferdata.h"

struct MapPersistent : public DrawableBufferData {

    virtual void init(const MeshData& mesh);
    virtual void render(const MeshData& mesh);
    virtual void update(const MeshData& mesh);
    virtual void shutdown();

    void* MappedVertPointer;
    unsigned int BufferNumber;
    unsigned int BufferCount;
    unsigned int Bytes;
};

