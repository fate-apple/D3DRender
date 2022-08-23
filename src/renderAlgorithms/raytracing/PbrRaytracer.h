#pragma once
#include "DxRaytracer.h"
#include "Material.hlsli"
#include "Raytracing.h"
#include "RaytracingBindingTable.h"
#include "rendering/pbr.h"

class PbrRaytracer : DxRaytracer
{
public:
    static RaytracingObjectType DefineObjectType(const ref<RaytracingBlas>& blas, const std::vector<ref<PbrMaterial>>& materials);
    void finalizeForRender();
protected:
    struct ShaderData{ // 32 byte, 8alignment
        PbrMaterialCB materialCB;
        DxCpuDescriptorHandle resources; // Vertex buffer, index buffer, PBR textures.
    };
    static inline uint32 numRayTypes = 0;
    RaytracingBindingTable<ShaderData> bindingTable;

    static inline DxPushableDescriptorHeap descriptorHeap;
    
    void Initialize(const wchar* shaderPath, uint32 maxPayloadSize, uint32 maxRecursionDepth, const D3D12_ROOT_SIGNATURE_DESC& globalDesc);

private:
    inline static uint32 instanceContributionToHitGroupIndex = 0;       //+= numGeometries * numRayTypes;
    bool dirty = false;
    static inline std::vector<PbrRaytracer*> registeredPbrRaytracer;

    static void PushTexture(const ref<DxTexture>& texture, uint32& flags, uint32 flag);
};
