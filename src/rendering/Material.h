#pragma once
#include "pch.h"
#include "dx/DxBuffer.h"
#include "dx/DxTexture.h"
#include "core/math.h"
#include "LightProbe.hlsli"

struct DxCommandList;

struct CommonMaterialInfo
{
    ref<DxTexture> sky;
    ref<DxTexture> irradiance;
    ref<DxTexture> environment;

    ref<DxTexture> aoTexture;
    ref<DxTexture> sssTexture;
    ref<DxTexture> ssrTexture;

    ref<DxTexture> lightProbeIrradiance;
    ref<DxTexture> lightProbeDepth;
    LightProbeGridCB lightProbeGrid;

    ref<DxTexture> tiledCullingGrid;
    ref<DxBuffer> tiledObjectsIndexList;
    ref<DxBuffer> tiledSpotLightIndexList;
    ref<DxBuffer> pointLightBuffer;
    ref<DxBuffer> spotLightBuffer;
    ref<DxBuffer> pointLightShadowInfoBuffer;
    ref<DxBuffer> spotLightShadowInfoBuffer;

    ref<DxBuffer> decalBuffer;
    ref<DxBuffer> decalTextureAtlas;

    ref<DxTexture> shadowMap;

    ref<DxTexture> volumetricsTexture;

    // These two are only set, if the material is rendered after the opaque pass.
    ref<DxTexture> opaqueDepth;
    ref<DxTexture> worldNormals;

    DxDynamicConstantBuffer cameraCBV;
    DxDynamicConstantBuffer sunCBV;

    float environmentIntensity;
    float skyIntensity;
};