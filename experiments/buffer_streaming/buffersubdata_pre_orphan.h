#pragma once
#include "drawablebufferdata.h"
class BufferSubDataPreOrphan : public DrawableBufferData
{
public:
    BufferSubDataPreOrphan();
    virtual ~BufferSubDataPreOrphan();
    virtual void render(const MeshData& mesh);
};