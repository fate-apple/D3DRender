#include "pch.h"
#include "RenderUtils.h"

#include "dx/DxContext.h"


static uint64 skinningFence;
void WaitForSkinningToFinish()
{
    if (skinningFence)
    {
        dxContext.renderQueue.WaitForOtherQueue(dxContext.computeQueue); // Wait for GPU skinning.
    }
}
