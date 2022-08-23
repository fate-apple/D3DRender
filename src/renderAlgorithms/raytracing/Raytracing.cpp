#include "pch.h"
#include "Raytracing.h"

RaytracingPipelineBuilder::RaytracingPipelineBuilder(const wchar* shaderFilename, uint32 payloadSize, uint32 maxRecursiveDespth,
                                                     bool hasMeshGeometry, bool hasProceduralGeometry)
{
    this->shaderFilename = shaderFilename;
    this->payloadSize = payloadSize;
    this->maxRecursionDepth = maxRecursionDepth;
    this->hasMeshGeometry = hasMeshGeometry;
    this->hasProceduralGeometry = hasProceduralGeometry;

    assert(hasMeshGeometry || hasProceduralGeometry);
}

RaytracingPipelineBuilder::RaytracingRootSignature RaytracingPipelineBuilder::CreateRaytracingRootSignature(const D3D12_ROOT_SIGNATURE_DESC& desc)
{
    RaytracingRootSignature result;
    result.rootSignature = CreateRootSignature(desc);
    result.rootSignaturePtr = result.rootSignature.rootSignature.Get();
    return result;
}

RaytracingPipelineBuilder& RaytracingPipelineBuilder::GlobalRootSignature(D3D12_ROOT_SIGNATURE_DESC desc)
{
    assert(!globalRs.rootSignature.rootSignature);
    globalRs = CreateRaytracingRootSignature(desc);
    SET_NAME(globalRs.rootSignature.rootSignature, "Global raytracing root signature");

    D3D12_STATE_SUBOBJECT& so = subObjects[numSubobjects++];
    so.pDesc = &globalRs.rootSignaturePtr;
    so.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;

    return *this;
}


static uint32 GetShaderBindingTableSize(const D3D12_ROOT_SIGNATURE_DESC& rootSignatureDesc)
{
    uint32 size = 0;
    for (uint32 i = 0; i < rootSignatureDesc.NumParameters; ++i)
    {
        if (rootSignatureDesc.pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
        {
            size += AlignTo(rootSignatureDesc.pParameters[i].Constants.Num32BitValues * 4, 8);
        } 
        else
        {
            size += 8;
        }
    }
    return size;
}

RaytracingPipelineBuilder& RaytracingPipelineBuilder::RayGen(const wchar* entryPoint, D3D12_ROOT_SIGNATURE_DESC desc)
{
    assert(!rayGenRs.rootSignature.rootSignature);
    rayGenEntryPoint = entryPoint;
    desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
    rayGenRs = CreateRaytracingRootSignature(desc);
    SET_NAME(rayGenRs.rootSignature.rootSignature, "Local raytracing root signature");
    
    D3D12_EXPORT_DESC& exp = exports[numExports++];
    exp.Name = entryPoint;
    exp.Flags = D3D12_EXPORT_FLAG_NONE;
    exp.ExportToRename = nullptr;

    {
        D3D12_STATE_SUBOBJECT& so = subObjects[numSubobjects++];
        so.pDesc = &rayGenRs.rootSignaturePtr;
        so.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
    }
    {
        D3D12_STATE_SUBOBJECT& so = subObjects[numSubobjects++];
        D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION& as = associations[numAssociations++];

        stringBuffer[numStrings++] = entryPoint;

        as.NumExports = 1;
        as.pExports = &stringBuffer[numStrings - 1];
        as.pSubobjectToAssociate = &subObjects[numSubobjects - 2];      //rayGenRs

        so.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
        so.pDesc = &as;
    }
    allExports.push_back(entryPoint);

    uint32 size = GetShaderBindingTableSize(desc);
    tableEntrySize = max(size, tableEntrySize);
    return *this;
}
