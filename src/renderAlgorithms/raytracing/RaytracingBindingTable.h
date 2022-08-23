#pragma once
#include "Raytracing.h"


template <typename TShaderData>
struct RaytracingBindingTable
{
    //pso, rs ,BindingTableDesc
    void Initiailize(const DxRaytracingPipeline* pipeline);
    //update hit information. ++current hit groups. reallocate if greater to maxNumHitGroup
    void Push(const TShaderData* shaderData);
    void Build();

    ref<DxBuffer> GetBuffer() { return bindingTableBuffer; }
    uint32 GetNumberOfHitGroups() { return currentHitGroup; }

private:
    struct alignas(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT) BindingTableEntry
    {
        uint8 identifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
        TShaderData shaderData;
    };
    void* bindingTable = nullptr;
    uint32 totalBindingTableSize;

    uint32 maxNumHitGroups;
    uint32 currentHitGroup;

    uint32 numRayTypes;
    BindingTableEntry* rayGen;
    BindingTableEntry* miss; //hitGroups.size()
    BindingTableEntry* hit;

    const DxRaytracingPipeline* pipeline;
    ref<DxBuffer> bindingTableBuffer;

    void Allocate();
};

template <typename TShaderData> void RaytracingBindingTable<TShaderData>::Allocate()
{
    // rayGen => RayGendata
    totalBindingTableSize = (uint32)(
        AlignTo(sizeof(BindingTableEntry), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT)
        + AlignTo(sizeof(BindingTableEntry) * numRayTypes, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT)
        + AlignTo(sizeof(BindingTableEntry) * numRayTypes * maxNumHitGroups, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT));
    if(!bindingTable) {
        bindingTable = _aligned_malloc(totalBindingTableSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
    } else {
        bindingTable = _aligned_realloc(bindingTable, totalBindingTableSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
    }
    assert(pipeline->shaderBindingTableDesc.rayGenOffset == 0);
    assert(
        pipeline->shaderBindingTableDesc.missOffset == AlignTo(sizeof(BindingTableEntry),
            D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT));
    assert(
        pipeline->shaderBindingTableDesc.hitOffset == AlignTo((1 + numRayTypes) * sizeof(BindingTableEntry),
            D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT));

    rayGen = static_cast<BindingTableEntry*>(bindingTable);
    miss = static_cast<BindingTableEntry*>(AlignTo(rayGen + sizeof(BindingTableEntry),
                                                   D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT));
    hit = static_cast<BindingTableEntry*>(AlignTo((1 + numRayTypes) * sizeof(BindingTableEntry),
                                                  D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT));
}

template <typename TShaderData> void RaytracingBindingTable<TShaderData>::Initiailize(const DxRaytracingPipeline* pipeline)
{
    this->pipeline = pipeline;
    assert(pipeline->shaderBindingTableDesc.entrySize == sizeof(BindingTableEntry));

    numRayTypes = static_cast<uint32>(pipeline->shaderBindingTableDesc.hitGroups.size());
    maxNumHitGroups = 32;
    Allocate();

    memcpy(rayGen->identifier, pipeline->shaderBindingTableDesc.rayGen, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    //ID3D12StateObjectProperties, GetShaderIdentifier(raygenEntryPoint);
    for(uint32 i(0); i < numRayTypes; ++i) {
        BindingTableEntry* m = miss + i;
        memcpy(m->identifier, pipeline->shaderBindingTableDesc.miss[i], D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    }
}

template <typename TShaderData> void RaytracingBindingTable<TShaderData>::Push(const TShaderData* shaderDatas)
{
    if(currentHitGroup >= maxNumHitGroups) {
        maxNumHitGroups *= 2;
        Allocate();
    }
    BindingTableEntry* base = hit + (numRayTypes * currentHitGroup);
    ++currentHitGroup;
    for(uint32 i(0); i < numRayTypes; ++i) {
        BindingTableEntry* h = base + i;
        memcpy(h->identifier, pipeline->shaderBindingTableDesc.hitGroups[i].mesh, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        h->shaderData = shaderDatas[i];
    }
}

template <typename TShaderData> void RaytracingBindingTable<TShaderData>::Build()
{
    bindingTableBuffer = CreateBuffer(1, max(totalBindingTableSize, 1u), bindingTable);
}
