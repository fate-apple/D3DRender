#pragma once
#include "dx/DxTexture.h"

//PbrMaterial info of a pixel stored in PbrMaterialCB
struct PbrMaterial
{
    PbrMaterial() = default;
    PbrMaterial(ref<DxTexture> albedo, 
        ref<DxTexture> normal, 
        ref<DxTexture> roughness, 
        ref<DxTexture> metallic, 
        const vec4& emission, 
        const vec4& albedoTint, 
        float roughnessOverride, 
        float metallicOverride,
        bool doubleSided,
        float uvScale)
        : albedo(albedo), 
        normal(normal), 
        roughness(roughness), 
        metallic(metallic), 
        emission(emission), 
        albedoTint(albedoTint), 
        roughnessOverride(roughnessOverride), 
        metallicOverride(metallicOverride),
        doubleSided(doubleSided),
        uvScale(uvScale) {}

    //PbrMaterialCB cb;
    ref<DxTexture> albedo;
    ref<DxTexture> normal;
    ref<DxTexture> roughness;
    ref<DxTexture> metallic;

    vec4 emission;
    vec4 albedoTint;
    float roughnessOverride;
    float metallicOverride;
    bool doubleSided;
    float uvScale;
};

struct PbrEnvironment
{
    ref<DxTexture> sky;
    ref<DxTexture> environment;
    ref<DxTexture> irradiance;

    fs::path name;
};