#include "pch.h"
#include "DxDynamicDescriptorHeap.h"

#include "DxContext.h"

void DxDynamicDescriptorHeap::Initialize(uint32 numDescriptorPerHeap)
{
    this->_numDescriptorPerHeap = numDescriptorPerHeap;
    _descriptorTableBitMask = 0;
    _staleDescriptorTableBitMask = 0;
    _currentCpuDescriptorHandle = DxCpuDescriptorHandle(D3D12_DEFAULT);
    _currentGpuDescriptorHandle = DxGpuDescriptorHandle(D3D12_DEFAULT);
    _numFreeHandles = 0;
    _descriptorHandleCaches.resize(numDescriptorPerHeap);
}


void DxDynamicDescriptorHeap::Reset()
{
    _freeDescriptorHeaps = _descriptorHeapPool;
    _currentDescriptorHeap.Reset();
    _currentCpuDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
    _currentGpuDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
    _numFreeHandles = 0;
    _descriptorTableBitMask = 0;
    _staleDescriptorTableBitMask = 0;
    for(uint32 i(0); i < maxDescriptorTables; ++i) {
        _descriptorTableCaches[i].Reset();
    }
}

/**
 * \brief compute Stale Descriptor Count by sum numDescriptors in _descriptorTableCaches[32]. use 32 bit _staleDescriptorTableBitMask to determine whether add cache in this index
 * \return num of Descriptor have been stored.
 */
uint32 DxDynamicDescriptorHeap::ComputeStaleDescriptorCount() const
{
    uint32 result = 0;
    DWORD i;
    DWORD staleDescriptorMask = _staleDescriptorTableBitMask;
    //LSB
    while(_BitScanForward(&i, staleDescriptorMask)) {
        result += _descriptorTableCaches[i].numDescriptors;
        unSetBit(staleDescriptorMask, i);
    }
    return result;
}

Dx12DescriptorHeap DxDynamicDescriptorHeap::CreateDescriptorHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = _numDescriptorPerHeap;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    Dx12DescriptorHeap result;
    //Creates a descriptor heap object.A descriptor heap is a collection of contiguous allocations of descriptors, one allocation for every descriptor.
    //Descriptor heaps contain many object types that are not part of a Pipeline State Object (PSO), such as Shader Resource Views (SRVs), Unordered Access Views (UAVs), Constant Buffer Views (CBVs), and Samplers.
    CheckResult(dxContext.device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&result)));
    return result;
}

Dx12DescriptorHeap DxDynamicDescriptorHeap::RequestDescriptorHeap()
{
    Dx12DescriptorHeap result;
    if(_freeDescriptorHeaps.size() > 0) {
        result = _freeDescriptorHeaps.back();
        _freeDescriptorHeaps.pop_back();
    } else {
        result = CreateDescriptorHeap();
        _descriptorHeapPool.push_back(result);
    }
    return result;
}


/**
 * \brief Commit stored  Descriptors once. simply create new DescriptorHeap if rest of current heap is not enough.
 * Scan Each stored descriptor table (range), copy to heap -> set rootParameter -> reset cache
 */
void DxDynamicDescriptorHeap::CommitStagedDescriptors(DxCommandList* commandList, bool isGraphics)
{
    uint32 numDescriptorsToCommit = ComputeStaleDescriptorCount();
    if(numDescriptorsToCommit > 0) {
        if(!_currentDescriptorHeap || _numFreeHandles < numDescriptorsToCommit) {
            _currentDescriptorHeap = RequestDescriptorHeap();
            _currentCpuDescriptorHandle = _currentDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
            _currentGpuDescriptorHandle = _currentDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
            _numFreeHandles = _numDescriptorPerHeap;

            commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, _currentDescriptorHeap);
            // When updating the descriptor heap on the command list, all descriptor
            // tables must be (re)recopied to the new descriptor heap (not just
            // the stale descriptor tables).
            _staleDescriptorTableBitMask = _descriptorTableBitMask;
        }
    }

    DWORD rootIndex;
    while(_BitScanForward(&rootIndex, _staleDescriptorTableBitMask)) {
        uint32 numSrcDescriptors = _descriptorTableCaches[rootIndex].numDescriptors;
        D3D12_CPU_DESCRIPTOR_HANDLE* srcDescriptorHandles = _descriptorTableCaches[rootIndex].baseDescriptor;
        D3D12_CPU_DESCRIPTOR_HANDLE destDescriptorRangeStart = _currentCpuDescriptorHandle;
        dxContext.device->CopyDescriptors(1, &destDescriptorRangeStart, &numSrcDescriptors,
                                          numSrcDescriptors, srcDescriptorHandles, nullptr,
                                          D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        if(isGraphics) {
            commandList->SetGraphicsDescriptorTable(
                rootIndex, static_cast<CD3DX12_GPU_DESCRIPTOR_HANDLE>(_currentGpuDescriptorHandle));
        } else {
            commandList->SetComputeDescriptorTable(
                rootIndex, static_cast<CD3DX12_GPU_DESCRIPTOR_HANDLE>(_currentGpuDescriptorHandle));
        }
        _currentCpuDescriptorHandle += numSrcDescriptors;
        _currentGpuDescriptorHandle += numSrcDescriptors;
        _numFreeHandles -= numSrcDescriptors;

        unSetBit(_staleDescriptorTableBitMask, rootIndex);
    }
}


void DxDynamicDescriptorHeap::CommitStagedDescriptorsForDraw(DxCommandList* commandList)
{
    CommitStagedDescriptors(commandList, true);
}

void DxDynamicDescriptorHeap::CommitStageDescriptorForCompute(DxCommandList* commandList)
{
    CommitStagedDescriptors(commandList, false);
}


void DxDynamicDescriptorHeap::SetCurrentDescriptorHeap(DxCommandList* commandList)
{
    commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, _currentDescriptorHeap);
}

// offset keep in rs directly
void DxDynamicDescriptorHeap::StageDescriptors(uint32 rootParameterIndex, uint32 offset, uint32 numDescriptors,
                                               DxCpuDescriptorHandle srcDescriptor)
{
    assert(rootParameterIndex < maxDescriptorTables);
    DescriptorTableCache& cache = _descriptorTableCaches[rootParameterIndex];
    assert((offset + numDescriptors) <= cache.numDescriptors);
    D3D12_CPU_DESCRIPTOR_HANDLE* dstDescriptor = cache.baseDescriptor + offset;
    for(uint32 i(0); i < numDescriptors; i++) {
        dstDescriptor[i] = srcDescriptor + i;
    }
    setBit(_staleDescriptorTableBitMask, rootParameterIndex);
}

void DxDynamicDescriptorHeap::ParseRootSignature(const DxRootSignature& rootSignature)
{
    //this caches staged descriptors that is configured to match the layout of a specific root signature. Reset if signature change
    _staleDescriptorTableBitMask = 0;
    _descriptorTableBitMask = rootSignature.tableRootParameterMask;

    uint32 bitmask = _descriptorTableBitMask;
    uint32 currentOffset = 0;
    DWORD rootIndex;
    uint32 descriptorTableIndex = 0;
    while(_BitScanForward(&rootIndex, bitmask)) {
        uint32 numDescriptors = rootSignature.descriptorTableSizes[descriptorTableIndex++];
        //reset cache
        auto& cache = _descriptorTableCaches[rootIndex];
        cache.numDescriptors = numDescriptors;
        cache.baseDescriptor = &_descriptorHandleCaches[currentOffset];
        currentOffset += numDescriptors;
        unSetBit(bitmask, rootIndex);
    }
    assert(currentOffset <= _numDescriptorPerHeap && "currentOffset > _numDescriptorPerHeap");
}
