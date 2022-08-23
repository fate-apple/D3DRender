#pragma once

#include "dx/DxBuffer.h"
#include "dx/DxPipeline.h"
#include "geometry/mesh_builder.h"



//https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#hit-group-table-indexing
enum RaytracingAsMode
{
    RaytracingAsRebuild,
    RaytracingAsRefit,
};

enum RaytracingGeometryType
{
    RaytracingMeshGeometry,
    RaytracingProceduralGeometry,
};

struct RaytracingBlasGeometry
{
    RaytracingGeometryType type;

    // Only valid for mesh geometry.
    VertexBufferGroup vertexBuffer;
    ref<DxIndexBuffer> indexBuffer;
    SubmeshInfo submesh;
};

struct RaytracingBlas
{
    ref<DxBuffer> scratch;
    ref<DxBuffer> blas;

    std::vector<RaytracingBlasGeometry> geometries;
};


struct RaytracingBlasBuilder
{
    RaytracingBlasBuilder& Push(VertexBufferGroup vertexBuffer, ref<DxIndexBuffer> indexBuffer, SubmeshInfo submesh,
                                bool opaque = true, const trs& localTransform = trs::identity);
    RaytracingBlasBuilder& Push(const std::vector<BoundingBox>& boundingBoxes, bool opaque);
    ref<RaytracingBlas> Finish(bool keepScratch = false);

private:
    std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs;

    std::vector<RaytracingBlasGeometry> geometries;

    std::vector<mat4> localTransforms; // For meshes.
    std::vector<D3D12_RAYTRACING_AABB> aabbDescs; // For procedurals.
};

struct RaytracingObjectType
{
    ref<RaytracingBlas> blas;
    uint32 instanceContributionToHitGroupIndex;
};

struct raytrace_component
{
    RaytracingObjectType type;
};


struct RaytracingsShader
{
    void* mesh;
    void* procedural;
};

struct RaytracingShaderBindingTableDesc
{
    uint32 entrySize;

    void* rayGen;
    std::vector<void*> miss;
    // equal to rayTypes per hitGroup
    std::vector<RaytracingsShader> hitGroups;

    uint32 rayGenOffset;
    uint32 missOffset;
    uint32 hitOffset;
};

//https://developer.nvidia.com/rtx/raytracing/dxr/dx12-raytracing-tutorial-part-1
struct DxRaytracingPipeline
{
    Dx12RaytracingPipelineState pipeline;
    DxRootSignature rootSignature;
    RaytracingShaderBindingTableDesc shaderBindingTableDesc;
};


struct RaytracingMeshHitgroup
{
    const wchar* closestHit; // Optional.
    const wchar* anyHit; // Optional.
};

struct RaytracingProceduralHitgroup
{
    const wchar* intersection;
    const wchar* closestHit; // Optional.
    const wchar* anyHit; // Optional.
};

struct RaytracingPipelineBuilder
{
public:
    RaytracingPipelineBuilder(const wchar* shaderFilename, uint32 payloadSize, uint32 maxRecursiveDespth, bool hasMeshGeometry,
                              bool hasProceduralGeometry);

    RaytracingPipelineBuilder& GlobalRootSignature(D3D12_ROOT_SIGNATURE_DESC desc);
    RaytracingPipelineBuilder& RayGen(const wchar* entryPoint, D3D12_ROOT_SIGNATURE_DESC desc = {});
    RaytracingPipelineBuilder& HitGroup(const wchar* groupName, const wchar* miss,
                                        RaytracingMeshHitgroup mesh, D3D12_ROOT_SIGNATURE_DESC meshRootSignatureDesc = {},
                                        RaytracingProceduralHitgroup procedural = {},
                                        D3D12_ROOT_SIGNATURE_DESC proceduralRootSignatureDesc = {});
    DxRaytracingPipeline Finish();
private:
    struct RaytracingRootSignature
    {
        DxRootSignature rootSignature;
        ID3D12RootSignature* rootSignaturePtr;      //equal to  DxRootSignature.rootSignature.get(), redundant. use to initial D3D12_STATE_SUBOBJECT.pdesc
    };

    RaytracingRootSignature globalRs;
    RaytracingRootSignature rayGenRs;
    const wchar* rayGenEntryPoint;
    std::vector<const wchar*> missEntryPoints;

    std::vector<const wchar*> emptyAssociations;
    std::vector<const wchar*> allExports;
    std::vector<const wchar*> shaderNameDefines;

    uint32 payloadSize;
    uint32 maxRecursionDepth;
    bool hasMeshGeometry;
    bool hasProceduralGeometry;
    uint32 tableEntrySize = 0;

    const wchar* shaderFilename;

    D3D12_STATE_SUBOBJECT subObjects[512];
    uint32 numSubobjects = 0;

    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION associations[16];
    uint32 numAssociations = 0;

    D3D12_HIT_GROUP_DESC hitGroups[8];
    uint32 numHitGroups = 0;

    D3D12_EXPORT_DESC exports[24];
    uint32 numExports = 0;

    const wchar* stringBuffer[128];
    uint32 numStrings = 0;

    RaytracingRootSignature rootSignatures[8];
    uint32 numRootSignatures = 0;

    std::wstring groupNameStorage[8];
    uint32 groupNameStoragePtr = 0;


    RaytracingRootSignature CreateRaytracingRootSignature(const D3D12_ROOT_SIGNATURE_DESC& desc);
};