#pragma once

#include <vector>

#include "common.h"
#include "meshdata.h"

#include "drawablebufferdata.h"

struct TrippleVBONoOrphan : public DrawableBufferData {

    struct RingBuffer {
        std::vector<unsigned int> VAOs;
        std::vector<unsigned int> VBOs;
        std::vector<GLsync> Syncs;
        std::vector<bool> VBO_Available;
    };

    unsigned int ActiveIndex;
    RingBuffer BufferObjects;

    virtual void init(const MeshData& mesh);
    virtual void render(const MeshData& mesh);
    virtual void update(const MeshData& mesh);
    virtual void shutdown();
};

