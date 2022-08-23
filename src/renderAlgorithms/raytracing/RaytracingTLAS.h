#pragma once
#include <d3d12.h>

#include "Raytracing.h"
#include "dx/DxBuffer.h"

//https://developer.nvidia.com/rtx/raytracing/dxr/dx12-raytracing-tutorial-part-1
struct RaytracingTLAS
{
    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> allInstances;
    RaytracingAsMode rebuildMode;
    ref<DxBuffer> scratch;
    ref<DxBuffer> tlas;

    void Initialize(RaytracingAsMode rebuildMode = RaytracingAsRebuild);
};
