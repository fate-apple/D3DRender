#pragma once
#include "Dx.h"
#include "DxBuffer.h"
#include "DxDynamicDescriptorHeap.h"
#include "DxPipeline.h"
#include "DxRenderTarget.h"
#include "DxTexture.h"

// used for render target. D3D12_RECT
struct ClearRect
{
    uint32 x, y, width, height;
};

struct DxCommandList
{
    explicit DxCommandList(D3D12_COMMAND_LIST_TYPE type);

    Dx12GraphicsCommandList commandList;
    D3D12_COMMAND_LIST_TYPE type;

    Dx12CommandAllocator commandAllocator;
    uint64 lastExecutionFenceValue = static_cast<uint64>(-1);
    DxCommandList* next = nullptr;
    DxDynamicDescriptorHeap dynamicDescriptorHeap;
    Dx12QueryHeap timeStampQueryHeap;

    //ID3D12DescriptorHeap only one for each type
    ID3D12DescriptorHeap* descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES]{};

    void Reset();

    //Defines constants that specify the state of a resource regarding how the resource is being used. COMMON, INDEX_BUFFER, RENDER_TARGET  etc.
    void TransitionBarrier(Dx12Resource resource, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to,
                           uint32 subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) const;
    void TransitionBarrier(const ref<DxTexture>& texture, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to,
                           uint32 subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) const;
    void TransitionBarrier(const ref<DxBuffer>& buffer, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to,
                           uint32 subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) const;

    //render target
    
    void ClearRTV(DxRtvDescriptorHandle rtv, float r, float g, float b, float a = 1.f, const ClearRect* rects = 0, uint32 numRects = 0);
    void ClearRTV(DxRtvDescriptorHandle rtv, const float* clearColor, const ClearRect* rects = 0, uint32 numRects = 0);
    void ClearRTV(const ref<DxTexture>& texture, float r, float g, float b, float a = 1.f, const ClearRect* rects = 0, uint32 numRects = 0);

    void ClearDepthAndStencil(DxDsvDescriptorHandle dsv, float depth = 1.f, uint32 stencil = 0, const ClearRect* rects = 0, uint32 numRects = 0);
    void ClearDepthAndStencil(const ref<DxTexture>& texture, float depth = 1.f, uint32 stencil = 0, const ClearRect* rects = 0, uint32 numRects = 0);

    //Barriers
    void Barriers(CD3DX12_RESOURCE_BARRIER* barriers, uint32 numBarriers);

    //PipeLine
    void SetPipelineState(Dx12PipelineState pipelineState);
    void SetPipelineState(Dx12RTPipelineState rtPipelineState);

    //Shader argument 
    void SetGraphicsRootSignature(const DxRootSignature& rootSignature);
    void SetGraphics32BitConstants(uint32 rootParameterIndex, uint32 numConstants, const void* constants);
    template <typename T>
    void SetGraphics32BitConstants(uint32 rootParameterIndex, const T& constants);

    void SetComputeRootSignature(const DxRootSignature& rootSignature);
    void SetCompute32BitConstants(uint32 rootParameterIndex, uint32 numConstants, const void* constants);
    template <typename T>
    void SetCompute32BitConstants(uint32 rootParameterIndex, const T& constants);

    void SetGraphicsDynamicConstantBuffer(uint32 rootParameterIndex, DxDynamicConstantBuffer address);
    void SetComputeDynamicConstantBuffer(uint32 rootParameterIndex, DxDynamicConstantBuffer address);

    void SetRootGraphicsSRV(uint32 rootParameterIndex, const ref<DxBuffer>& buffer);
    void SetRootGraphicsSRV(uint32 rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS address);
    void SetRootComputeSRV(uint32 rootParameterIndex, const ref<DxBuffer>& buffer);
    void SetRootComputeSRV(uint32 rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS address);

    void SetRootGraphicsUAV(uint32 rootParameterIndex, const ref<DxBuffer>& buffer);
    void SetRootGraphicsUAV(uint32 rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS address);
    void SetRootComputeUAV(uint32 rootParameterIndex, const ref<DxBuffer>& buffer);
    void SetRootComputeUAV(uint32 rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS address);
    
    void SetDescriptorHeapResource(uint32 rootParameterIndex, uint32 offset, uint32 count, DxCpuDescriptorHandle handle);
    void SetDescriptorHeapSRV(uint32 rootParameterIndex, uint32 offset, DxCpuDescriptorHandle handle)
    {
        SetDescriptorHeapResource(rootParameterIndex, offset, 1, handle);
    }
    void SetDescriptorHeapSRV(uint32 rootParameterIndex, uint32 offset, const ref<DxTexture>& texture)
    {
        SetDescriptorHeapResource(rootParameterIndex, offset, 1, texture->defaultSRV);
    }
    void SetDescriptorHeapSRV(uint32 rootParameterIndex, uint32 offset, const ref<DxBuffer>& buffer)
    {
        SetDescriptorHeapResource(rootParameterIndex, offset, 1, buffer->defaultSRV);
    }

    void SetDescriptorHeapUAV(uint32 rootParameterIndex, uint32 offset, DxCpuDescriptorHandle handle)
    {
        SetDescriptorHeapResource(rootParameterIndex, offset, 1, handle);
    }
    void SetDescriptorHeapUAV(uint32 rootParameterIndex, uint32 offset, const ref<DxTexture>& texture)
    {
        SetDescriptorHeapResource(rootParameterIndex, offset, 1, texture->defaultUAV);
    }
    void SetDescriptorHeapUAV(uint32 rootParameterIndex, uint32 offset, const ref<DxBuffer>& buffer)
    {
        SetDescriptorHeapResource(rootParameterIndex, offset, 1, buffer->defaultUAV);
    }

    //Shader parameters
    void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, Dx12DescriptorHeap descriptorHeap);
    void SetDescriptorHeap(DxDescriptorRange& descriptorPage);
    void ResetToDynamicDescriptorHeap();
    void SetGraphicsDescriptorTable(uint32 rootParameterIndex, CD3DX12_GPU_DESCRIPTOR_HANDLE handle);
    void SetComputeDescriptorTable(uint32 rootParameterIndex, CD3DX12_GPU_DESCRIPTOR_HANDLE handle);
    
    void ClearUAV(const ref<DxBuffer>& buffer, float val = 0.f);
    void ClearUAV(Dx12Resource resource, DxCpuDescriptorHandle cpuHandle, DxGpuDescriptorHandle gpuHandle, float val = 0.f);
    void ClearUAV(Dx12Resource resource, DxCpuDescriptorHandle cpuHandle, DxGpuDescriptorHandle gpuHandle, uint32 val = 0);


    //IA
    void SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY topology);
    void SetVertexBuffer(uint32 slot, const ref<DxVertexBuffer>& buffer);
    void SetVertexBuffer(uint32 slot, const DxDynamicVertexBuffer& dynamicVertexBuffer);
    void SetVertexBuffer(uint32 slot, const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView);
    void SetIndexBuffer(const ref<DxIndexBuffer>& buffer);
    void SetIndexBuffer(const DxDynamicIndexBuffer& dynamicIndexBuffer);
    void SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& indexBufferView);

    //Rasterizer
    void SetViewport(const D3D12_VIEWPORT& dxViewPort);
    void SetViewport(float x, float y, float width, float height, float minDepth = 0.f, float maxDepth = 1.f);
    void SetScissor(const D3D12_RECT& scissor);


    //OM
    void SetRenderTarget(const DxRtvDescriptorHandle* rtvDescriptorHandle, uint32 numRtvs,
                         const DxDsvDescriptorHandle* dsvDescriptorHandle);
    void SetRenderTarget(const DxRenderTarget& renderTarget);

    //Draw,
    void Draw(uint32 vertexCount, uint32 instanceCount, uint32 startVertex, uint32 startInstance);
    void DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 startIndex, uint32 baseVertex, uint32 startInstance);
    void DrawIndirect(Dx12CommandSignature commandSignature, uint32 numCommands, const Dx12Resource& commandBuffer,
                      uint32 commandBufferOffset = 0);
    void DrawFullScreenTriangle();
    void DrawCubeTriangleStrip();

    //Dispatch
    void Dispatch(uint32 numGroupsX, uint32 numGroupsY = 1, uint32 numGroupsZ = 1);
};
//https://stackoverflow.com/questions/495021/why-can-templates-only-be-implemented-in-the-header-file

template <typename T>
void DxCommandList::SetGraphics32BitConstants(uint32 rootParameterIndex, const T& constants)
{
    static_assert(sizeof(T) % 4 == 0, "Size of type must be a multiple of 32 bits.");
    SetGraphics32BitConstants(rootParameterIndex, sizeof(T) / 4, &constants);
}


template <typename T>
void DxCommandList::SetCompute32BitConstants(uint32 rootParameterIndex, const T& constants)
{
    static_assert(sizeof(T) % 4 == 0, "Size of type must be a multiple of 32 bits.");
    SetCompute32BitConstants(rootParameterIndex, sizeof(T) / 4, &constants);
}
