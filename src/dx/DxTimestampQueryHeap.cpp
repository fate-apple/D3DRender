#include "pch.h"
#include "DxTimestampQueryHeap.h"

#include "DxContext.h"

void DxTimestampQueryHeap::Initialize(uint32 maxCount)
{
    D3D12_QUERY_HEAP_DESC desc;
    desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP; //OCCLUSION,  PIPELINE_STATISTICS
    desc.Count = maxCount;
    desc.NodeMask = 0;

    CheckResult(dxContext.device->CreateQueryHeap(&desc, IID_PPV_ARGS(&heap)));
}
