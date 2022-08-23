#include "pch.h"
#include "DxContext.h"

#include "DxPipeline.h"
#include "DxProfiling.h"

// Allocate dynamically on the heap to ensure that this thing never gets destroyed before anything else. This is quite a hack.
DxContext& dxContext = *new DxContext{};

static void enableDebugLayer()
{
#if defined(_DEBUG)
    com<ID3D12Debug3> debugInterface;
    CheckResult(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
    debugInterface->EnableDebugLayer();
    //TODO: sjw. update windows kit to enable ID3D12Debug5 & SetEnableAutoDebugName
#endif
}

static DxGIFactory CreateFactory()
{
    DxGIFactory dxGiFactory;
    // ReSharper disable once CppInitializedValueIsAlwaysRewritten
    uint32 createFactoryFlags = 0;
#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
    CheckResult(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxGiFactory)));
    return dxGiFactory;
}

struct GIAdapterDesc
{
    DxGIAdapter adapter;
    D3D_FEATURE_LEVEL featureLevel;
};

static GIAdapterDesc GetAdapterDesc(const DxGIFactory& factory, D3D_FEATURE_LEVEL minimumFeatureLevel = D3D_FEATURE_LEVEL_12_0)
{
    com<IDXGIAdapter1> dxGiAdapter1;
    DxGIAdapter dxGiAdapter4;
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_9_1;
    constexpr D3D_FEATURE_LEVEL possibleFeatureLevels[] =
    {
        D3D_FEATURE_LEVEL_9_1,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_12_1
    };
    uint32 firstFeatureLevel = 0;
    for(uint32 i(0); i < arraySize(possibleFeatureLevels); ++i) {
        if(possibleFeatureLevels[i] == minimumFeatureLevel) {
            firstFeatureLevel = i;
            break;
        }
    }
    uint64 maxVideoMemory = 0;
    for(int i(0); factory->EnumAdapters1(i, &dxGiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
        dxGiAdapter1->GetDesc1(&dxgiAdapterDesc1);
        // Check to see if the adapter can create a D3D12 device without actually creating it. using "uuidof(ID3D12Device)" instead of actual  globally unique identifier (GUID) for the device interface.
        if((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0) {
            D3D_FEATURE_LEVEL adapterFeatureLevel = D3D_FEATURE_LEVEL_9_1;
            bool supportFeatureLevel = false;

            for(auto j(firstFeatureLevel); j < arraySize(possibleFeatureLevels); ++j) {
                if(SUCCEEDED(
                    D3D12CreateDevice(dxGiAdapter1.Get(), possibleFeatureLevels[j], __uuidof(ID3D12Device), nullptr))) {
                    // NOLINT(clang-diagnostic-language-extension-token)
                    adapterFeatureLevel = possibleFeatureLevels[j];
                    supportFeatureLevel = true;
                }
            }
            if(supportFeatureLevel && dxgiAdapterDesc1.DedicatedVideoMemory > maxVideoMemory) {
                maxVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
                featureLevel = adapterFeatureLevel;
                CheckResult(dxGiAdapter1.As(&dxGiAdapter4));
            }
        }
    }
    return {dxGiAdapter4, featureLevel};
}

static Dx12Device CreateDevice(const DxGIAdapter& adapter, D3D_FEATURE_LEVEL featureLevel)
{
    Dx12Device device;
    CheckResult(D3D12CreateDevice(adapter.Get(), featureLevel, IID_PPV_ARGS(&device)));

#if defined(_DEBUG)
    com<ID3D12InfoQueue> infoQueue;
    if(SUCCEEDED(device.As(&infoQueue))) {
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
        D3D12_MESSAGE_SEVERITY severities[] = {
            D3D12_MESSAGE_SEVERITY_INFO
        };
        D3D12_MESSAGE_ID ids[] = {
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE, // This warning occurs when using capture frame while graphics debugging.
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
        };
        D3D12_INFO_QUEUE_FILTER filter = {};
        filter.DenyList.NumSeverities = arraySize(severities);
        filter.DenyList.pSeverityList = severities;
        filter.DenyList.NumIDs = arraySize(ids);
        filter.DenyList.pIDList = ids;
        CheckResult(infoQueue->PushStorageFilter(&filter));
    }
#endif

    return device;
}

static DxFeatureSupportDesc CheckFeatureSupport(const Dx12Device& device)
{
    DxFeatureSupportDesc featureSupportDesc = {};
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
    if(SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5)))) {
        featureSupportDesc.raytracingTier = static_cast<DxRaytracingTier>(options5.RaytracingTier);
    }
#if ADVANCED_GPU_FEATURES_ENABLED
    //mesh shader
    D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
    if(SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7)))) {
        featureSupportDesc.meshShaderTier = static_cast<DxMeshShaderTier>(options7.MeshShaderTier);
    } else {
        std::cerr << "Checking support for mesh shader feature failed.";
    }
#endif
    return featureSupportDesc;
}

bool DxContext::Initialize()
{
    // enables applications to access new DPI-related scaling behaviors on a per top-level window basis.
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    //vectored handlers are called in the order that they were added, after the debugger gets a first chance notification, but before the system begins unwinding the stack.
    //TODO: sjw
    AddVectoredExceptionHandler(TRUE, nullptr);
    //Initializes the COM library for use by the calling thread, sets the thread's concurrency model, and creates a new apartment for the thread if one is required
    // Apartment model & Don't use DDE for Ole1 support.
    CheckResult(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE));
    enableDebugLayer();

    factory = CreateFactory();
    auto [adapter, featureLevel] = GetAdapterDesc(factory);
    this->dxgiAdapter = adapter;
    if(!dxgiAdapter) {
        //TODO: sjw. log system. uniform log/warning/error 
        std::cerr << "No suitable gpu found.\n";
        return false;
    }
    this->device = CreateDevice(dxgiAdapter, featureLevel);
    featureSupport = CheckFeatureSupport(device);
    bufferedFrameID = NUM_BUFFERED_FRAMES - 1;
    descriptorHandleIncrementSizeCBV_SRV_UAV = device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    srvUavHeap.Initialize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, false, KB(4));
    srvUavHeapShaderVisible.Initialize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true, 128);
    rtvHeap.Initialize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false, KB(1));
    dsvHeap.Initialize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, false, KB(1));

    for(uint32 i(0); i < NUM_BUFFERED_FRAMES; i++) {
        _pagePools[i].Initialize(MB(2));

#if ENABLE_DX_PROFILING
        _timeStampQueryHeap[i].Initialize(MAX_NUM_DX_PROFILE_EVENTS);
        _resolvedTimeStampBuffers[i] = CreateReadBackBuffer(MAX_NUM_DX_PROFILE_EVENTS, sizeof(uint64));
#endif
    }

    frameUploadBuffer.Reset();
    frameUploadBuffer.pagePool = &_pagePools[bufferedFrameID];
    _pagePools[bufferedFrameID].Reset();

    renderQueue.Initialize(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    computeQueue.Initialize(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    copyQueue.Initialize(device, D3D12_COMMAND_LIST_TYPE_DIRECT);

    D3D12_INDIRECT_ARGUMENT_DESC argumentDesc;
    argumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH; //draw, constant, dispatch
    dxCommandSignature = CreateCommandSignature({}, &argumentDesc, 1, sizeof(D3D12_DISPATCH_ARGUMENTS));

    return true;
}


void DxContext::quit()
{
#if ENABLE_DX_PROFILING
    for (uint32 b = 0; b < NUM_BUFFERED_FRAMES; ++b)
    {
        _resolvedTimeStampBuffers[b].reset();
    }
#endif

    running = false;
    FlushApplication();
    WaitForSingleObject(renderQueue.processThreadHandle, INFINITE);
    WaitForSingleObject(computeQueue.processThreadHandle, INFINITE);
    WaitForSingleObject(copyQueue.processThreadHandle, INFINITE);

    for (uint32 b = 0; b < NUM_BUFFERED_FRAMES; ++b)
    {
        _textureGraveyard[b].clear();
        _bufferGraveyard[b].clear();
        _objectGraveyard[b].clear();

        for (auto allocation : _allocationGraveyard[b])
        {
            allocation->Release();
        }
        _allocationGraveyard[b].clear();
    }
}


void DxContext::FlushApplication()
{
    renderQueue.Flush();
    computeQueue.Flush();
    copyQueue.Flush();
}


void DxContext::endFrame(DxCommandList* cl)
{
// #if ENABLE_DX_PROFILING
//     dxProfilingFrameEndMarker(cl);
//     cl->commandList->ResolveQueryData(timestampHeaps[bufferedFrameID].heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0, timestampQueryIndex[bufferedFrameID], resolvedTimestampBuffers[bufferedFrameID]->resource.Get(), 0);
// #endif
}

void DxContext::newFrame(uint64 frameID)
{
    this->frameID = frameID;

    _mutex.lock();
    bufferedFrameID = (uint32)(frameID % NUM_BUFFERED_FRAMES);

// #if ENABLE_DX_PROFILING
//     uint64* timestamps = (uint64*)mapBuffer(resolvedTimestampBuffers[bufferedFrameID], true);
//     dxProfilingResolveTimeStamps(timestamps);
//     unmapBuffer(resolvedTimestampBuffers[bufferedFrameID], false);
//
//     timestampQueryIndex[bufferedFrameID] = 0;
// #endif


    _textureGraveyard[bufferedFrameID].clear();
    _bufferGraveyard[bufferedFrameID].clear();
    _objectGraveyard[bufferedFrameID].clear();
    for (auto allocation : _allocationGraveyard[bufferedFrameID])
    {
        allocation->Release();
    }
    _allocationGraveyard[bufferedFrameID].clear();


    frameUploadBuffer.Reset();
    frameUploadBuffer.pagePool = &_pagePools[bufferedFrameID];

    _pagePools[bufferedFrameID].Reset();
    frameDescriptorAllocator.NewFrame(bufferedFrameID);

    _mutex.unlock();
}


void DxContext::Retire(D3D12MA::Allocation* allocation)
{
    _mutex.lock();
    _allocationGraveyard[bufferedFrameID].push_back(allocation);
    _mutex.unlock();
}

void DxContext::Retire(BufferGrave&& bufferGrave)
{
    _mutex.lock();
    _bufferGraveyard[bufferedFrameID].push_back(std::move(bufferGrave));
    _mutex.unlock();
}

void DxContext::Retire(const Dx12Object& object)
{
    _mutex.lock();
    _objectGraveyard[bufferedFrameID].push_back(object);
    _mutex.unlock();
}

void DxContext::Retire(TextureGrave&& textureGrave)
{
    _mutex.lock();
    _textureGraveyard[bufferedFrameID].push_back(std::move(textureGrave));
    _mutex.lock();
}


DxCommandQueue& DxContext::GetQueue(D3D12_COMMAND_LIST_TYPE type)
{
    return type == D3D12_COMMAND_LIST_TYPE_DIRECT
               ? renderQueue
               : type == D3D12_COMMAND_LIST_TYPE_COMPUTE
               ? computeQueue
               : copyQueue;
}

DxCommandList* DxContext::GetFreeCommandList(DxCommandQueue& commandQueue) const
{
    commandQueue.mutex.lock();
    DxCommandList* cl = commandQueue.freeCommandLists;
    if(cl) commandQueue.freeCommandLists = cl->next;
    commandQueue.mutex.unlock();
    //No free command list 
    if(!cl) {
        cl = new DxCommandList(commandQueue.commandListType);
        AtomicIncrement(commandQueue.numTotalCommandLists);
    }
#if ENABLE_DX_PROFILING
    cl->timeStampQueryHeap = _timeStampQueryHeap[bufferedFrameID].heap;
#endif
    return cl;
}

DxCommandList* DxContext::GetFreeRenderCommandList()
{
    return GetFreeCommandList(renderQueue);
}

DxCommandList* DxContext::GetFreeComputeCommandList()
{
    return GetFreeCommandList(computeQueue);
}


DxCommandList* DxContext::GetFreeCopyCommandList()
{
    return GetFreeCommandList(copyQueue);
}

static void EnQueueRunningCommandList(DxCommandQueue& commandQueue, DxCommandList* cl)
{
    cl->next = nullptr;
    if(commandQueue.newestRunningCommandList) {
        assert(commandQueue.newestRunningCommandList->next == nullptr);
        commandQueue.newestRunningCommandList->next = cl;
    }
    commandQueue.newestRunningCommandList = cl;
    if(!commandQueue.runningCommandLists) {
        commandQueue.runningCommandLists = cl;
    }
}

uint64 DxContext::ExecuteCommandList(DxCommandList* cl)
{
    DxCommandQueue& commandQueue = GetQueue(cl->type);
    //End record to CommandAllocator, wait for actually submit commands to commandQueue
    //result Indicates that recording to the command list has finished.
    CheckResult(cl->commandList->Close());
    ID3D12CommandList* dxCommandList = cl->commandList.Get(); //cannot apply & to r-value
    commandQueue.commandQueue->ExecuteCommandLists(1, &dxCommandList);

    const uint64 fenceValue = commandQueue.Signal();
    cl->lastExecutionFenceValue = fenceValue;

    commandQueue.mutex.lock();
    EnQueueRunningCommandList(commandQueue, cl);
    AtomicIncrement(commandQueue.numRunningCommandLists);
    commandQueue.mutex.unlock();

    return fenceValue;
}

uint64 DxContext::ExecuteCommandLists(DxCommandList** commandLists, uint32 count)
{
    DxCommandQueue& dxCommandQueue = GetQueue(commandLists[0]->type);
    ID3D12CommandList* iD3D12CommandList[16];

    //TODO: sjw. commandList count > 16 case?
    for(uint32 i(0); i < count; ++i) {
        CheckResult(commandLists[i]->commandList->Close());
        iD3D12CommandList[i] = commandLists[i]->commandList.Get();
    }
    dxCommandQueue.commandQueue->ExecuteCommandLists(count, iD3D12CommandList);
    uint64 fencevalue = dxCommandQueue.Signal();
    for(uint32 i(0); i < count; ++i) {
        commandLists[i]->lastExecutionFenceValue = fencevalue;
    }

    dxCommandQueue.mutex.lock();
    for(uint32 i(0); i < count; ++i) {
        EnQueueRunningCommandList(dxCommandQueue, commandLists[i]);
    }
    AtomicAdd(dxCommandQueue.numRunningCommandLists, count);
    dxCommandQueue.mutex.unlock();
    return fencevalue;
}

DxAllocation DxContext::AllocateDynamicBuffer(uint32 sizeInBytes, uint32 alignment)
{
    DxAllocation allocation = frameUploadBuffer.Allocate(sizeInBytes, alignment);
    return allocation;
}


std::pair<DxDynamicConstantBuffer, void*> DxContext::UploadDynamicConstantBuffer(uint32 sizeInBytes, const void* data)
{
    DxAllocation allocation = AllocateDynamicBuffer(sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    memcpy(allocation.cpuPtr, data, sizeInBytes);
    return { {allocation.gpuPtr}, allocation.cpuPtr};
}
