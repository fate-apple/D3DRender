#pragma once
#include "Dx.h"
#include "DxBuffer.h"
#include "DxCommandList.h"
#include "DxCommandQueue.h"
#include "DxDescriptorAllocation.h"
#include "DxTexture.h"
#include "DxTimestampQueryHeap.h"
#include "DxUploadBuffer.h"
#include "d3d12memoryallocator/D3D12MemAlloc.h"

enum class DxRaytracingTier
{
    None,
    DxRaytracing1_0,
    DxRaytracing1_1,
};

enum class DxMeshShaderTier
{
    None,
    DxMeshShader1_0
};

struct DxFeatureSupportDesc
{
    DxRaytracingTier  raytracingTier;
    DxMeshShaderTier meshShaderTier;

    bool Raytracing() {return raytracingTier >= DxRaytracingTier::DxRaytracing1_0;}
};

struct DxContext
{
    DxContext() = default;
    
    uint64 frameID{};
    uint32 bufferedFrameID{};     //bufferedFrameID = NUM_BUFFERED_FRAMES - 1 when start;
#if ENABLE_DX_PROFILING
    volatile uint32 timestampQueryIndex[NUM_BUFFERED_FRAMES]{};
#endif

    DxGIFactory factory;
    DxGIAdapter dxgiAdapter;
    Dx12Device device;

    //different commandQueue control their own commandLists of same type;
    DxCommandQueue renderQueue;
    DxCommandQueue computeQueue;
    DxCommandQueue copyQueue;

    DxFeatureSupportDesc featureSupport{};

    DxDescriptorHeap srvUavHeap;
    DxDescriptorHeap srvUavHeapShaderVisible;       // Used for resources
    DxDescriptorHeap rtvHeap;
    DxDescriptorHeap dsvHeap;

    DxUploadBuffer frameUploadBuffer{};
    DxFrameDescriptorAllocator frameDescriptorAllocator;
    D3D12MA::Allocator* memoryAllocator{};
    
    uint32 descriptorHandleIncrementSizeCBV_SRV_UAV{};
    Dx12CommandSignature dxCommandSignature;
    volatile bool running = true;

    bool Initialize();
    void quit();
    void FlushApplication();

    void newFrame(uint64 frameID);
    void endFrame(DxCommandList* cl);
    //move resource to Graveyard 
    void Retire(D3D12MA::Allocation* allocation);
    void Retire(BufferGrave&& bufferGrave);
    void Retire(const Dx12Object& resource);
    void Retire(TextureGrave&& textureGrave);

    DxCommandList* GetFreeCopyCommandList();
    DxCommandList* GetFreeComputeCommandList();
    DxCommandList* GetFreeRenderCommandList();
    uint64 ExecuteCommandList(DxCommandList* cl);
    uint64 ExecuteCommandLists(DxCommandList** commandLists, uint32 count);

    DxAllocation AllocateDynamicBuffer(uint32 sizeInBytes, uint32 alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    std::pair<DxDynamicConstantBuffer, void*> UploadDynamicConstantBuffer(uint32 sizeInBytes, const void* data);
    template< typename T>
    DxDynamicConstantBuffer UploadDynamicConstantBuffer( const T& data)
    {
        return UploadDynamicConstantBuffer(sizeof(T), &data).first;
    }

private:
#if ENABLE_DX_PROFILING
    DxTimestampQueryHeap _timeStampQueryHeap[NUM_BUFFERED_FRAMES];
    ref<DxBuffer> _resolvedTimeStampBuffers[NUM_BUFFERED_FRAMES];
#endif

    DxPagePool _pagePools[NUM_BUFFERED_FRAMES] = {};

    std::mutex _mutex;
    //TODO: sjw. use one in whole swapChain
    std::vector<TextureGrave> _textureGraveyard[NUM_BUFFERED_FRAMES];
    std::vector<BufferGrave> _bufferGraveyard[NUM_BUFFERED_FRAMES];
    std::vector<Dx12Object> _objectGraveyard[NUM_BUFFERED_FRAMES];
    std::vector<D3D12MA::Allocation*> _allocationGraveyard[NUM_BUFFERED_FRAMES];    // [C2248] “D3D12MA::Allocation::~Allocation”: 无法访问 private 成员(在“D3D12MA::Allocation”类中声明)
    
    DxCommandQueue& GetQueue(D3D12_COMMAND_LIST_TYPE type);
    DxCommandList* GetFreeCommandList(DxCommandQueue& queue) const;
};


extern DxContext& dxContext;