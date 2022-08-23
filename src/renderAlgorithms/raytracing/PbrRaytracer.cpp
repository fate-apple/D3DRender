#include "pch.h"
#include "PbrRaytracer.h"

RaytracingObjectType PbrRaytracer::DefineObjectType(const ref<RaytracingBlas>& blas,
                                                    const std::vector<ref<PbrMaterial>>& materials)
{
    uint32 numGeometries = (uint32)blas->geometries.size();
    ShaderData* hitData = reinterpret_cast<ShaderData*>(alloca(sizeof(ShaderData) * numRayTypes));

    for(uint32 i(0); i < numGeometries; ++i) {
        assert(blas->geometries[i].type = RaytracingMeshGeometry);

        SubmeshInfo submeshInfo = blas->geometries[i].submesh;
        const ref<PbrMaterial>& material = materials[i];
        DxCpuDescriptorHandle base = descriptorHeap.currentCpu;
        descriptorHeap.Push().CreateBufferSRV(blas->geometries[i].vertexBuffer.others,
                                              BufferRange{submeshInfo.baseVertex, submeshInfo.numVertices});
        descriptorHeap.Push().CreateRawBufferSRV(blas->geometries[i].indexBuffer,
                                                 BufferRange{submeshInfo.firstIndex, submeshInfo.numIndices});

        uint32 flags = 0;
        PushTexture(material->albedo, flags, USE_ALBEDO_TEXTURE);
        PushTexture(material->normal, flags, USE_NORMAL_TEXTURE);
        PushTexture(material->roughness, flags, USE_ROUGHNESS_TEXTURE);
        PushTexture(material->metallic, flags, USE_METALLIC_TEXTURE);

        if(blas->geometries[i].indexBuffer->elementSize == 4) {
            flags |= USE_32_BIT_INDICES;
        }
        // set information Gpu should use 
        hitData[0].materialCB.Initialize(
            material->albedoTint,
            material->emission.xyz,
            material->roughnessOverride,
            material->metallicOverride,
            flags
        );
        hitData[0].resources = base;
        // total 2 shader for now; The other shader types don't need any data, so we don't set it here.
        for(auto rt : registeredPbrRaytracer) {
            rt->bindingTable.Push(hitData);
        }
    }
    RaytracingObjectType result = {blas, instanceContributionToHitGroupIndex};
    instanceContributionToHitGroupIndex += numGeometries * numRayTypes;
    for(auto rt : registeredPbrRaytracer) {
        rt->dirty = true;
    }
    return result;
}

void PbrRaytracer::finalizeForRender()
{
    if(dirty) {
        bindingTable.Build();
        dirty = false;
    }
}

void PbrRaytracer::Initialize(const wchar* shaderPath, uint32 maxPayloadSize, uint32 maxRecursionDepth,
                              const D3D12_ROOT_SIGNATURE_DESC& globalDesc)
{
    CD3DX12_DESCRIPTOR_RANGE hitSrvRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6, 0, 1);
    CD3DX12_ROOT_PARAMETER hitRootParameters[] = {
        RootConstants<PbrMaterialCB>(0, 1),
        RootDescriptorTable(1, &hitSrvRange),
    };
    D3D12_ROOT_SIGNATURE_DESC hitDesc = {arraySize(hitRootParameters), hitRootParameters}
    RaytracingMeshHitgroup radianceHitgroup = {L"radianceClosestHit"};
    RaytracingMeshHitgroup shadowHitgroup = {};
    pipeline = RaytracingPipelineBuilder(shaderPath, maxPayloadSize, maxRecursionDepth, true, false, )
               .GlobalRootSignature(globalDesc)
               .RayGen(L"rayGen")
               .HitGroup(L"RADIANCE", L"radianceMiss", radianceHitgroup, hitDesc)
               .HitGroup(L"SHADOW", L"shadowMiss", shadowHitgroup)
               .Finish();
    assert(numRayTypes == 0 || numRayTypes == static_cast<uint32>(pipeline.shaderBindingTableDesc.hitGroups.size()));
    numRayTypes = static_cast<uint32>(pipeline.shaderBindingTableDesc.hitGroups.size());
    bindingTable.Initiailize(&pipeline);
    if (!descriptorHeap.descriptorHeap)
    {
        descriptorHeap.Initialize(2048); // TODO.
    }
    registeredPbrRaytracer.push_back(this);
}

void PbrRaytracer::PushTexture(const ref<DxTexture>& texture, uint32& flags, uint32 flag)
{
    if(texture) {
        descriptorHeap.Push().Create2DTextureSRV(texture);
        flags |= flag;
    }
    else {
        descriptorHeap.Push().CreateNullTextureSRV();
    }
}
