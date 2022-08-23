#pragma once
#include "core/math.h"
#include "dx/DxBuffer.h"
#include "geometry/mesh_builder.h"

struct StaticDepthOnlyRenderCommand
{
    mat4 transform;
    DxVertexBufferView vertexBufferView;
    DxIndexBufferView indexBufferView;
    SubmeshInfo submeshInfo;
    uint32 objectId;
};

struct DynamicDepthOnlyRenderCommand
{
    mat4 transform;
    mat4 prevFrameTransform;
    DxVertexBufferView vertexBufferView;
    DxIndexBufferView indexBuffer;
    SubmeshInfo submesh;
    uint32 objectId;
};

// add prevFrame vertexBufferView & VertexBufferAddress
struct AnimatedDepthOnlyRenderCommand
{
    mat4 transform;
    mat4 prevFrameTransform;
    DxVertexBufferView vertexBufferView;
    D3D12_GPU_VIRTUAL_ADDRESS prevFrameVertexBufferAddress;
    DxIndexBufferView indexBufferView;
    SubmeshInfo submeshInfo;
    uint32 objectId;
};

//same to depth only except objectID
struct OutlineRenderCommand
{
    mat4 transform;
    DxVertexBufferView vertexBufferView;
    DxIndexBufferView indexBufferView;
    SubmeshInfo submeshInfo;
};

struct ShadowRenderCommand
{
    mat4 transform;
    DxVertexBufferView vertexBufferView;
    DxIndexBufferView indexBufferView;
    SubmeshInfo submeshInfo;
};