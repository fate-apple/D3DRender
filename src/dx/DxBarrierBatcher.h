#pragma once
#include "Dx.h"
#include "DxCommandList.h"

struct BarrierBatcher
{
    DxCommandList* cl;
    CD3DX12_RESOURCE_BARRIER barriers[16];
    uint32 numBarriers = 0;
    
    BarrierBatcher(DxCommandList* cl);
    ~BarrierBatcher(){Submit();}
    void Submit();

    BarrierBatcher& Transition(const Dx12Resource& res, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to, uint32 subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
    BarrierBatcher& Transition(const ref<DxTexture>& res, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to, uint32 subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
    BarrierBatcher& Transition(const ref<DxBuffer>& res, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to);

    BarrierBatcher& Uav(const Dx12Resource& res);
    BarrierBatcher& Uav(const ref<DxTexture>& res);
    BarrierBatcher& Uav(const ref<DxBuffer>& res);
};