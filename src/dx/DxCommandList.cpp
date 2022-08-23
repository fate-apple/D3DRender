#include "pch.h"
#include "DxCommandList.h"

#include "DxContext.h"

DxCommandList::DxCommandList(D3D12_COMMAND_LIST_TYPE type)
{
    this->type = type;
    CheckResult(dxContext.device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));
    CheckResult(dxContext.device->CreateCommandList(0, type, commandAllocator.Get(), nullptr,
                                                    IID_PPV_ARGS(&commandList)));
    dynamicDescriptorHeap.Initialize();
}

void DxCommandList::Reset()
{
    commandAllocator->Reset();
    CheckResult(commandList->Reset(commandAllocator.Get(), nullptr));

    for(uint32 i(0); i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i) {
        descriptorHeaps[i] = nullptr;
    }

    dynamicDescriptorHeap.Reset();
}

void DxCommandList::TransitionBarrier(Dx12Resource resource, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to,
                                      uint32 subResource) const
{
    const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), from, to, subResource);
    commandList->ResourceBarrier(1, &barrier);
}

void DxCommandList::TransitionBarrier(const ref<DxTexture>& texture, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to,
                                      uint32 subResource) const
{
    TransitionBarrier(texture->resource, from, to, subResource);
}

void DxCommandList::TransitionBarrier(const ref<DxBuffer>& buffer, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to,
                                      uint32 subResource) const
{
    TransitionBarrier(buffer->resource, from, to, subResource);
}

/*-------------------------------Render Target---------------------------------------------------*/

static void getD3D12Rects(D3D12_RECT* d3rects, const ClearRect* rects, uint32 numRects)
{
    for(uint32 i = 0; i < numRects; ++i) {
        d3rects[i] = {
            (LONG)rects[i].x, (LONG)rects[i].y, (LONG)rects[i].x + (LONG)rects[i].width,
            (LONG)rects[i].y + (LONG)rects[i].height
        };
    }
}

void DxCommandList::ClearRTV(DxRtvDescriptorHandle rtv, const float* clearColor, const ClearRect* rects,
                               uint32 numRects)
{
    D3D12_RECT* d3rects = nullptr;
    if(numRects) {
        d3rects = (D3D12_RECT*)alloca(sizeof(D3D12_RECT) * numRects);
        getD3D12Rects(d3rects, rects, numRects);
    }

    commandList->ClearRenderTargetView(rtv, clearColor, numRects, d3rects);
}

void DxCommandList::ClearRTV(DxRtvDescriptorHandle rtv, float r, float g, float b, float a, const ClearRect* rects,
                               uint32 numRects)
{
    float clearColor[] = {r, g, b, a};
    ClearRTV(rtv, clearColor, rects, numRects);
}


void DxCommandList::ClearRTV(const ref<DxTexture>& texture, float r, float g, float b, float a, const ClearRect* rects,
                               uint32 numRects)
{
    ClearRTV(texture->defaultRTV, r, g, b, a, rects, numRects);
}


void DxCommandList::ClearDepthAndStencil(DxDsvDescriptorHandle dsv, float depth, uint32 stencil, const ClearRect* rects,
                                           uint32 numRects)
{
    D3D12_RECT* d3rects = 0;
    if(numRects) {
        d3rects = (D3D12_RECT*)alloca(sizeof(D3D12_RECT) * numRects);
        getD3D12Rects(d3rects, rects, numRects);
    }

    commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil, numRects,
                                       d3rects);
}

void DxCommandList::ClearDepthAndStencil(const ref<DxTexture>& texture, float depth, uint32 stencil,
                                           const ClearRect* rects, uint32 numRects)
{
    ClearDepthAndStencil(texture->defaultDSV, depth, stencil, rects, numRects);
}

void DxCommandList::Barriers(CD3DX12_RESOURCE_BARRIER* barriers, uint32 numBarriers)
{
    commandList->ResourceBarrier(numBarriers, barriers);
}


/*-------------------------------Pipeline---------------------------------------------------*/
void DxCommandList::SetPipelineState(Dx12PipelineState pipelineState)
{
    //Sets all shaders and programs most of the fixed-function state of the graphics processing unit (GPU) pipeline.
    commandList->SetPipelineState(pipelineState.Get());
}

void DxCommandList::SetPipelineState(Dx12RTPipelineState rtPipelineState)
{
    // In the current release, SetPipelineState1 is only used for setting raytracing pipeline state.
    commandList->SetPipelineState1(rtPipelineState.Get());
}

void DxCommandList::SetGraphicsRootSignature(const DxRootSignature& rootSignature)
{
    dynamicDescriptorHeap.ParseRootSignature(rootSignature);
    commandList->SetGraphicsRootSignature(rootSignature.rootSignature.Get());
}

void DxCommandList::SetComputeRootSignature(const DxRootSignature& rootSignature)
{
    dynamicDescriptorHeap.ParseRootSignature(rootSignature);
    commandList->SetComputeRootSignature(rootSignature.rootSignature.Get());
}


void DxCommandList::SetGraphics32BitConstants(uint32 rootParameterIndex, uint32 numConstants, const void* constants)
{
    commandList->SetGraphicsRoot32BitConstants(rootParameterIndex, numConstants, constants, 0);
}


void DxCommandList::SetCompute32BitConstants(uint32 rootParameterIndex, uint32 numConstants, const void* constants)
{
    commandList->SetComputeRoot32BitConstants(rootParameterIndex, numConstants, constants, 0);
}

void DxCommandList::SetGraphicsDynamicConstantBuffer(uint32 rootParameterIndex, DxDynamicConstantBuffer address)
{
    //In Shader Model 4, shader constants are stored in one or more buffer resources in memory.
    //They can be organized into two types of buffers: constant buffers (cbuffers) and texture buffers (tbuffers).
    //Constant buffers are optimized for constant-variable usage, which is characterized by lower-latency access and more frequent update from the CPU. For this reason, additional size, layout, and access restrictions apply to these resources.
    //Texture buffers are accessed like textures and perform better for arbitrarily indexed data.
    //Regardless of which type of resource you use, there is no limit to the number of constant buffers or texture buffers an application can create.
    commandList->SetGraphicsRootConstantBufferView(rootParameterIndex, address.gpuPtr);
}

void DxCommandList::SetComputeDynamicConstantBuffer(uint32 rootParameterIndex, DxDynamicConstantBuffer address)
{
    commandList->SetComputeRootConstantBufferView(rootParameterIndex, address.gpuPtr);
}


void DxCommandList::SetRootGraphicsSRV(uint32 rootParameterIndex, const ref<DxBuffer>& buffer)
{
    //Sets a CPU descriptor handle for the shader resource in the graphics root signature.
    commandList->SetGraphicsRootShaderResourceView(rootParameterIndex, buffer->gpuVirtualAddress);
}

void DxCommandList::SetRootGraphicsSRV(uint32 rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS address)
{
    commandList->SetGraphicsRootShaderResourceView(rootParameterIndex, address);
}

void DxCommandList::SetRootComputeSRV(uint32 rootParameterIndex, const ref<DxBuffer>& buffer)
{
    commandList->SetComputeRootShaderResourceView(rootParameterIndex, buffer->gpuVirtualAddress);
}

void DxCommandList::SetRootComputeSRV(uint32 rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS address)
{
    commandList->SetComputeRootShaderResourceView(rootParameterIndex, address);
}

void DxCommandList::SetDescriptorHeapResource(uint32 rootParameterIndex, uint32 offset, uint32 count,
                                              DxCpuDescriptorHandle handle)
{
    dynamicDescriptorHeap.StageDescriptors(rootParameterIndex, offset, count, handle);
}

void DxCommandList::SetRootGraphicsUAV(uint32 rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS address)
{
    commandList->SetGraphicsRootUnorderedAccessView(rootParameterIndex, address);
}
void DxCommandList::SetRootComputeUAV(uint32 rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS address)
{
    commandList->SetComputeRootUnorderedAccessView(rootParameterIndex, address);
}

void DxCommandList::SetRootGraphicsUAV(uint32 rootParameterIndex, const ref<DxBuffer>& buffer)
{
    commandList->SetComputeRootUnorderedAccessView(rootParameterIndex, buffer->gpuVirtualAddress);
}


void DxCommandList::SetRootComputeUAV(uint32 rootParameterIndex, const ref<DxBuffer>& buffer)
{
    commandList->SetComputeRootUnorderedAccessView(rootParameterIndex, buffer->gpuVirtualAddress);
}

/*-------------------------------Shader parameters---------------------------------------------------*/
void DxCommandList::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, Dx12DescriptorHeap descriptorHeap)
{
    descriptorHeaps[type] = descriptorHeap.Get();
    uint32 numDescriptorHeaps = 0;
    ID3D12DescriptorHeap* heapsToSet[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES]{};
    for(uint32 i(0); i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; i++) {
        ID3D12DescriptorHeap* heap = descriptorHeaps[i];
        if(heap) {
            heapsToSet[numDescriptorHeaps++] = heap;
        }
    }
    //all the heaps being used are set in 
    commandList->SetDescriptorHeaps(numDescriptorHeaps, heapsToSet);
}

void DxCommandList::SetDescriptorHeap(DxDescriptorRange& descriptorRange)
{
    descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = descriptorRange.descriptorHeap.Get();
    uint32 numDescriptorHeaps = 0;
    ID3D12DescriptorHeap* heapsToSet[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES]{};
    for(uint32 i(0); i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; i++) {
        ID3D12DescriptorHeap* heap = descriptorHeaps[i];
        if(heap) {
            heapsToSet[numDescriptorHeaps++] = heap;
        }
    }
    commandList->SetDescriptorHeaps(numDescriptorHeaps, heapsToSet);
}

void DxCommandList::ResetToDynamicDescriptorHeap()
{
    dynamicDescriptorHeap.SetCurrentDescriptorHeap(this);
}

void DxCommandList::SetGraphicsDescriptorTable(uint32 rootParameterIndex, CD3DX12_GPU_DESCRIPTOR_HANDLE handle)
{
    //Sets a descriptor table into the graphics root signature.
    commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, handle);
}

void DxCommandList::SetComputeDescriptorTable(uint32 rootParameterIndex, CD3DX12_GPU_DESCRIPTOR_HANDLE handle)
{
    // Sets a descriptor table into the compute root signature.
    commandList->SetComputeRootDescriptorTable(rootParameterIndex, handle);
}

/**
 * \brief 
 * \param resource A pointer to the ID3D12Resource interface that represents the unordered-access-view (UAV) resource to clear.
 * \param cpuHandle A D3D12_CPU_DESCRIPTOR_HANDLE in a non-shader visible descriptor heap that references an initialized descriptor for the unordered-access view (UAV) that is to be cleared.
 * \param gpuHandle A D3D12_GPU_DESCRIPTOR_HANDLE that references an initialized descriptor for the unordered-access view (UAV) that is to be cleared.
 * This descriptor must be in a shader-visible descriptor heap, which must be set on the command list via SetDescriptorHeaps.
 * \param val 
 */
void DxCommandList::ClearUAV(Dx12Resource resource, DxCpuDescriptorHandle cpuHandle, DxGpuDescriptorHandle gpuHandle, float val)
{
    float vals[] = {val, val, val, val};
    commandList->ClearUnorderedAccessViewFloat(gpuHandle, cpuHandle, resource.Get(), vals, 0, nullptr);
}

void DxCommandList::ClearUAV(const ref<DxBuffer>& buffer, float val)
{
    ClearUAV(buffer->resource, buffer->cpuClearUAV, buffer->gpuClearUAV, val);
}

void DxCommandList::ClearUAV(Dx12Resource resource, DxCpuDescriptorHandle cpuHandle, DxGpuDescriptorHandle gpuHandle, uint32 val)
{
    uint32 vals[] = {val, val, val, val};
    commandList->ClearUnorderedAccessViewUint(gpuHandle, cpuHandle, resource.Get(), vals, 0, nullptr);
}

/*-------------------------------IA---------------------------------------------------*/

void DxCommandList::SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY topology)
{
    commandList->IASetPrimitiveTopology(topology);
}

void DxCommandList::SetVertexBuffer(uint32 slot, const ref<DxVertexBuffer>& buffer)
{
    //Bind an array of vertex buffers to the input-assembler stage.
    commandList->IASetVertexBuffers(slot, 1, &buffer->view);
}

void DxCommandList::SetVertexBuffer(uint32 slot, const DxDynamicVertexBuffer& dynamicVertexBuffer)
{
    commandList->IASetVertexBuffers(slot, 1, &dynamicVertexBuffer.view);
}

void DxCommandList::SetVertexBuffer(uint32 slot, const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView)
{
    commandList->IASetVertexBuffers(slot, 1, &vertexBufferView);
}

void DxCommandList::SetIndexBuffer(const ref<DxIndexBuffer>& buffer)
{
    commandList->IASetIndexBuffer(&buffer->view);
}

void DxCommandList::SetIndexBuffer(const DxDynamicIndexBuffer& dynamicIndexBuffer)
{
    commandList->IASetIndexBuffer(&dynamicIndexBuffer.view);
}

void DxCommandList::SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& indexBufferView)
{
    commandList->IASetIndexBuffer(&indexBufferView);
}


/*-------------------------------Rs---------------------------------------------------*/

void DxCommandList::SetViewport(const D3D12_VIEWPORT& dxViewPort)
{
    commandList->RSSetViewports(1, &dxViewPort);
}

void DxCommandList::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth)
{
    const D3D12_VIEWPORT dxViewport{x, y, width, height, minDepth, maxDepth};
    SetViewport(dxViewport);
}

void DxCommandList::SetScissor(const D3D12_RECT& scissor)
{
    commandList->RSSetScissorRects(1, &scissor);
}




void DxCommandList::SetRenderTarget(const DxRtvDescriptorHandle* rtvDescriptorHandle, uint32 numRtvs,
                                    const DxDsvDescriptorHandle* dsvDescriptorHandle)
{
    //TODO: sjw. better cast to remove warning
    //Sets CPU descriptor handles  Output merger (OM) for the render targets and depth stencil.
    commandList->OMSetRenderTargets(numRtvs, (D3D12_CPU_DESCRIPTOR_HANDLE*)(rtvDescriptorHandle), FALSE,
                                    (D3D12_CPU_DESCRIPTOR_HANDLE*)(dsvDescriptorHandle));
}

void DxCommandList::SetRenderTarget(const DxRenderTarget& renderTarget)
{
    SetRenderTarget(renderTarget.rtv, renderTarget.numAttachments, renderTarget.dsv ? &renderTarget.dsv : nullptr);
}


/*-------------------------------Draw---------------------------------------------------*/
void DxCommandList::Draw(uint32 vertexCount, uint32 instanceCount, uint32 startVertex, uint32 startInstance)
{
    dynamicDescriptorHeap.CommitStagedDescriptorsForDraw(this);
    //Draws non-indexed, instanced primitives.
    commandList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);
}

void DxCommandList::DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 startIndex, uint32 baseVertex,
                                uint32 startInstance)
{
    dynamicDescriptorHeap.CommitStagedDescriptorsForDraw(this);
    //Draws indexed, instanced primitives.
    commandList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, static_cast<INT>(baseVertex), startInstance);
}

void DxCommandList::DrawIndirect(Dx12CommandSignature commandSignature, uint32 numCommands, const Dx12Resource& commandBuffer,
                                 uint32 commandBufferOffset)
{
    dynamicDescriptorHeap.CommitStagedDescriptorsForDraw(this);
    //Apps perform indirect draws/dispatches using the ExecuteIndirect method.
    commandList->ExecuteIndirect(commandSignature.Get(), numCommands, commandBuffer.Get(), commandBufferOffset, 0, 0);
}

void DxCommandList::DrawFullScreenTriangle()
{
    dynamicDescriptorHeap.CommitStagedDescriptorsForDraw(this);
    SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Draw(3, 1, 0, 0);
}


void DxCommandList::DrawCubeTriangleStrip()
{
    dynamicDescriptorHeap.CommitStagedDescriptorsForDraw(this);
    SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    Draw(14, 1, 0, 0);
}

/*-------------------------------Dispatch---------------------------------------------------*/
void DxCommandList::Dispatch(uint32 numGroupsX, uint32 numGroupsY, uint32 numGroupsZ)
{
    dynamicDescriptorHeap.CommitStageDescriptorForCompute(this);
    commandList->Dispatch(numGroupsX, numGroupsY, numGroupsZ);
}
