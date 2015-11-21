#pragma once
#include "drawablebufferdata.h"
class BufferSubDataNoOrphan : public DrawableBufferData
{
public:
    BufferSubDataNoOrphan();
    virtual ~BufferSubDataNoOrphan();
    virtual void render(const MeshData& mesh);
};