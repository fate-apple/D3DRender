#include "pch.h"
#include  "DxCommandQueue.h"

#include <queue>
#include "DxContext.h"
#include "core/Thread.h"

static DWORD __stdcall ProcessRunningCommandLists(void* data)
{
    DxCommandQueue& commandQueue = *static_cast<DxCommandQueue*>(data);
    while(dxContext.running) {
        while(true) {
            commandQueue.mutex.lock();
            DxCommandList* list = commandQueue.runningCommandLists;
            if(list) {
                commandQueue.runningCommandLists = list->next;
                if(list == commandQueue.newestRunningCommandList) {
                    assert(list->next == nullptr);
                    commandQueue.newestRunningCommandList = nullptr;
                }
            }
            commandQueue.mutex.unlock();
            if(list) {
                commandQueue.WaitForFence(list->lastExecutionFenceValue);
                list->Reset();

                commandQueue.mutex.lock();
                list->next = commandQueue.freeCommandLists;
                commandQueue.freeCommandLists = list;
                AtomicDecrement(commandQueue.numRunningCommandLists);
                commandQueue.mutex.unlock();
            } else {
                break;
            }
        }
        SwitchToThread(); //yield
    }
    return 0;
}

void DxCommandQueue::Initialize(const Dx12Device& device, D3D12_COMMAND_LIST_TYPE type)
{
    fenceValue = 0;
    commandListType = type;

    D3D12_COMMAND_QUEUE_DESC desc{};
    desc.Type = type;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;

    CheckResult(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&commandQueue)));
    CheckResult(device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    //This method is used to determine the rate at which the GPU timestamp counter increments.
    if(SUCCEEDED(commandQueue->GetTimestampFrequency(&timeStampFrequency))) {
        //TODO: sjw. Calibrate
    }
    //Creates a thread to execute within the virtual address space of the calling process.
    //third parameter is the pointer to the application-defined function to be executed by the thread;
    //  DxCommandQueue.cpp(27): [C2664] “HANDLE CreateThread(LPSECURITY_ATTRIBUTES,SIZE_T,LPTHREAD_START_ROUTINE,LPVOID,DWORD,LPDWORD)”: 无法将参数 3 从“DWORD (__cdecl *)(void *)”转换为“LPTHREAD_START_ROUTINE”
    //note "(__cdecl *)" conflict; add __stdcall solved.
    processThreadHandle = CreateThread(0, 0, ProcessRunningCommandLists, this, 0, 0);       //
    switch(type) {
    case D3D12_COMMAND_LIST_TYPE_DIRECT: SET_NAME(commandQueue, "Render command queue")
        break;
    case D3D12_COMMAND_LIST_TYPE_COMPUTE: SET_NAME(commandQueue, "Compute command queue")
        break;
    case D3D12_COMMAND_LIST_TYPE_COPY: SET_NAME(commandQueue, "Copy command queue")
        break;
    case D3D12_COMMAND_LIST_TYPE_BUNDLE:
    case D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE:
    case D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS:
    case D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE:
        std::cerr << "not implement command list type";
        break;
    }
}


bool DxCommandQueue::IsFenceComplete(const uint64 inFenceValue) const
{
    return fence->GetCompletedValue() >= inFenceValue;
}


void DxCommandQueue::WaitForFence(uint64 inFenceValue) const
{
    if(!IsFenceComplete(inFenceValue)) {
        HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        assert(fenceEvent && "Failed to create fence event");

        fence->SetEventOnCompletion(fenceValue, fenceEvent);
        WaitForSingleObject(fenceEvent, DWORD_MAX);
        CloseHandle(fenceEvent);
    }
}

//负责为每一帧CPU提交给GPU的命令做划分.当CPU端使用CommandQueue调用Signal(fence, n)时，便会在GPU的跑道上设置一个标记为n的栅栏
//CPU端使用ID3D12Fence调用GetCompletedValue()时，便会返回当前GPU执行到的任务点的后一个栅栏的标记。
uint64 DxCommandQueue::Signal()
{
    uint64 signalFenceValue = AtomicIncrement(signalFenceValue)+1;      //fencevalue after atomic++
    CheckResult(commandQueue->Signal(fence.Get(), signalFenceValue));
    return signalFenceValue;
}

void DxCommandQueue::Flush()
{
    while (numRunningCommandLists) {}

    WaitForFence(Signal());
}


void DxCommandQueue::WaitForOtherQueue(DxCommandQueue& other)
{
    //The value that the command queue is waiting for the fence to reach or exceed.
    //So when ID3D12Fence::GetCompletedValue is greater than or equal to Value, the wait is terminated.
    commandQueue->Wait(other.fence.Get(), other.Signal());
}