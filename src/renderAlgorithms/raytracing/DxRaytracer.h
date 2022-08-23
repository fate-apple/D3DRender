#pragma once
#include "dx/DxDescriptor.h"

class DxRaytracer
{
protected:
    DxRaytracingPipeline pipeline;
    DxCpuDescriptorHandle resourceCpuBase[NUM_BUFFERED_FRAMES];
    DxGpuDescriptorHandle resourceGPUBase[NUM_BUFFERED_FRAMES];
    
};
