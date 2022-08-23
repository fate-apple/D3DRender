#pragma once

#include <dx/d3dx12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <sdkddkver.h>
#include "pch.h"

#define NUM_BUFFERED_FRAMES 2
#define ADVANCED_GPU_FEATURES_ENABLED 1

#define SET_NAME(obj, name) CheckResult((obj)->SetName(L##name));

//Represents the allocations of storage for graphics processing unit (GPU) commands. used with commandList(ID3D12GraphicsCommandList)
using Dx12CommandAllocator =  com<ID3D12CommandAllocator>;
//Enables creating Microsoft DirectX Graphics Infrastructure (DXGI) objects. Interface between API & Screen/Window, similar with WSI（Windows System Interface) in vulkan
using DxGIFactory =  com<IDXGIFactory4>;
//The IDXGIAdapter interface represents a display subsystem (including one or more GPUs, DACs and video memory).
//view DxGIAdapter as GPU hardware, and view Dx12Device as software API when beginning
using DxGIAdapter = com<IDXGIAdapter4>;
//Represents a virtual adapter; it is used to create command allocators, command lists, command queues, fences, resources,
//pipeline state objects, heaps, root signatures, samplers, and many resource views.
using Dx12Device = com<ID3D12Device5>;
using Dx12DescriptorHeap = com<ID3D12DescriptorHeap>;
//Encapsulates a generalized ability of the CPU and GPU to read and write to physical memory, or heaps.
//It contains abstractions for organizing and manipulating simple arrays of data as well as multidimensional data optimized for shader sampling.
using Dx12Resource = com<ID3D12Resource2>;
//A command signature object enables apps to specify indirect drawing, including the buffer format, command type and resource bindings to be used.
using Dx12CommandSignature = com<ID3D12CommandSignature>;
//A query heap holds an array of queries, referenced by indexes
using Dx12QueryHeap = com<ID3D12QueryHeap>;
//An interface from which ID3D12Device and ID3D12DeviceChild inherit from.It provides methods to associate private data and annotate object names.
//Sets application-defined data to a device object and associates that data with an application-defined GUID.
using Dx12Object = com<ID3D12Object>;
//Encapsulates a list of graphics commands for rendering. Includes APIs for instrumenting the command list execution, and for setting and clearing the pipeline state.
using Dx12GraphicsCommandList = com<ID3D12GraphicsCommandList6>;
//Represents the state of all currently set shaders as well as certain fixed function state objects. PSO.
using Dx12PipelineState = com<ID3D12PipelineState>;
//Represents a variable amount of configuration state,
//including shaders, that an application manages as a single unit and which is given to a driver atomically to process, such as compile or optimize.
using Dx12RTPipelineState = com<ID3D12StateObject>;
using Dx12RaytracingPipelineState = com<ID3D12StateObject>;
//This interface is used to return data of arbitrary length. Compute Shader use Blob to generate ID3D12PipelineState
//Blobs can be used as data buffers. Blobs can also be used for storing vertex, adjacency, and material information during mesh optimization, and for loading operations
//Also, these objects are used to return object code and error messages in APIs that compile vertex, geometry, and pixel shaders.
using Dx12Blob = com<ID3DBlob>;
using Dx12Swapchain =  com<IDXGISwapChain4>;




//use d3d12memoryallocator in ext.
namespace D3D12MA { class Allocator; class Allocation; };