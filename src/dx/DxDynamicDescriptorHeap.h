#pragma once
#include "Dx.h"
//#include "DxCommandList.h"
#include "DxDescriptorAllocation.h"

struct DxCommandList;
struct DxRootSignature;
//The DynamicDescriptorHeap class caches staged descriptors in a descriptor cache that is configured to match the layout of the root signature
//used by commandList. SRV,UAV,BCV only , shader visible;
struct DxDynamicDescriptorHeap
{
public:
    void Initialize(uint32 numDescriptorPerHeap = 1024);
    void Reset();
    
    void CommitStagedDescriptorsForDraw(DxCommandList* commandList);
    void CommitStageDescriptorForCompute(DxCommandList* commandList);
    void SetCurrentDescriptorHeap(DxCommandList* commandList);

    void StageDescriptors(uint32 rootParameterIndex, uint32 offset, uint32 numDescriptors, DxCpuDescriptorHandle srcDescriptor);
    void ParseRootSignature(const DxRootSignature& rootSignature);
private:
    static constexpr uint32 maxDescriptorTables = 32;

    //total handles (can be) stored in this heap;
    uint32 _numDescriptorPerHeap = 0;
    struct DescriptorTableCache
    {
        void Reset()
        {
            numDescriptors = 0;
            baseDescriptor = nullptr;
        }
        uint32 numDescriptors = 0;
        D3D12_CPU_DESCRIPTOR_HANDLE* baseDescriptor = nullptr;
    };

    //cache stored in _staleDescriptorTableBitMask. DescriptorTable is actually a range in descriptorHeap
    //bit index equal to rootParameterIndex. we store descriptor with this order in heap ,so don't store offset
    DescriptorTableCache _descriptorTableCaches[maxDescriptorTables];
    // a contiguous storage where cached cpu Handles actually stored
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> _descriptorHandleCaches;

    // Each bit in the bit mask represents the index in the root signature that contains a descriptor table.
    uint32 _descriptorTableBitMask = 0;
    // used When updating the descriptor heap on the command list. indicates descriptorHeap in i th rootParameter  has been updated;
    uint32 _staleDescriptorTableBitMask = 0;

    //stack
    std::vector<Dx12DescriptorHeap> _descriptorHeapPool;
    std::vector<Dx12DescriptorHeap> _freeDescriptorHeaps;

    Dx12DescriptorHeap _currentDescriptorHeap;
    DxGpuDescriptorHandle _currentGpuDescriptorHandle = {};
    DxCpuDescriptorHandle _currentCpuDescriptorHandle = {};

    uint32 _numFreeHandles = 0;

    uint32 ComputeStaleDescriptorCount() const;
    void CommitStagedDescriptors(DxCommandList* commandList, bool isGraphics);
    Dx12DescriptorHeap RequestDescriptorHeap();
    Dx12DescriptorHeap CreateDescriptorHeap();
};