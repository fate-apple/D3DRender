#include "pch.h"
#include "DxBarrierBatcher.h"

void BarrierBatcher::Submit()
{
    if (numBarriers)
    {
        cl->Barriers(barriers, numBarriers);
        numBarriers = 0;
    }
}

BarrierBatcher& BarrierBatcher::Transition(const Dx12Resource& res, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to,
                                           uint32 subresource)
{
    if(numBarriers == arraySize(barriers)) {
        Submit();
    }
    if(from != to) {
        barriers[numBarriers++] = CD3DX12_RESOURCE_BARRIER::Transition(res.Get(), from, to, subresource);
    }
    return *this;
}

BarrierBatcher& BarrierBatcher::Transition(const ref<DxBuffer>& res, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to)
{
    if(!res) return *this;
    return Transition(res->resource, from, to);
}

BarrierBatcher& BarrierBatcher::Transition(const ref<DxTexture>& res, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to,
                                           uint32 subresource)
{
    if(!res) return *this;
    return Transition(res->resource, from, to, subresource);
}

BarrierBatcher& BarrierBatcher::Uav(const Dx12Resource& res)
{
    if(numBarriers == arraySize(barriers)) {
        Submit();
    }

    //Creates an unordered-access-view (UAV) for the resource. Parameters:
    barriers[numBarriers++] = CD3DX12_RESOURCE_BARRIER::UAV(res.Get());

    return *this;
}

BarrierBatcher& BarrierBatcher::Uav(const ref<DxBuffer>& res)
{
    if(!res) return *this;
    return Uav(res->resource);
}

BarrierBatcher& BarrierBatcher::Uav(const ref<DxTexture>& res)
{
    if(!res) return *this;
    return Uav(res->resource);
}