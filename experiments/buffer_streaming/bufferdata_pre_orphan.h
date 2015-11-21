#pragma once
#include "drawablebufferdata.h"
class BufferdataPreOrphan : public DrawableBufferData
{
public:
    BufferdataPreOrphan();
    virtual ~BufferdataPreOrphan();
    virtual void render(const MeshData& mesh);
};