#pragma once
#include "drawablebufferdata.h"
class BufferSubDataPostOrphan : public DrawableBufferData
{
public:
    BufferSubDataPostOrphan();
    virtual ~BufferSubDataPostOrphan();
    virtual void render(const MeshData& mesh);
};