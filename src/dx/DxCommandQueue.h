#pragma once
#include <mutex>

#include "Dx.h"
#include "DxCommandList.h"

//Warper for ID3D12CommandQueue
struct DxCommandQueue
{
    com<ID3D12CommandQueue> commandQueue;
    uint64 timeStampFrequency{};
    D3D12_COMMAND_LIST_TYPE commandListType = D3D12_COMMAND_LIST_TYPE_DIRECT;
    com<ID3D12Fence> fence;
    volatile uint64 fenceValue{};

    DxCommandList* runningCommandLists{};
    DxCommandList* newestRunningCommandList{};
    DxCommandList* freeCommandLists{};

    volatile uint32 numRunningCommandLists{};
    volatile uint32 numTotalCommandLists{};
    
    HANDLE processThreadHandle{};
    std::mutex mutex;
    
    void Initialize(const Dx12Device& device, D3D12_COMMAND_LIST_TYPE type);
    bool IsFenceComplete(uint64 inFenceValue) const;
    void WaitForFence(uint64 inFenceValue) const;
    uint64 Signal();
    void Flush();
    void WaitForOtherQueue(DxCommandQueue& other);
};
