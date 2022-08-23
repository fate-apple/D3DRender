// ReSharper disable CppClangTidyClangDiagnosticGnuZeroVariadicMacroArguments
#pragma once
#include "core/math.h"
#include "dx/DxCommandList.h"
#include "dx/DxRenderTarget.h"
#include "RenderPass.h"
#include "core/reflect.h"
#include  "renderAlgorithms/AO/AmbientOcclusion.h"
#include "renderAlgorithms/lightCulling/LightCulling.h""
#include "renderAlgorithms/IBL/ImageBasedLighting.h"
#include "renderAlgorithms/AA/AntiAliasing.h"
#include "renderAlgorithms/postProcess/PostProcessing.h"
#include "renderAlgorithms/reflection/RaytracingSpecular.h"




void LoadCommonShaders();

void TextureSky(DxCommandList* cl,
                const DxRenderTarget& renderTarget, const mat4& proj, const mat4& view, ref<DxTexture> skyTextureCube,
                float intensity);
void DepthPrePass(DxCommandList* cl,
                  const DxRenderTarget& depthOnlyRenderTarget,
                  const OpaqueRenderPass* opaqueRenderPass,
                  const mat4& viewProj, const mat4& prevFrameViewProj,
                  vec2 jitter, vec2 preFrameJitter);

void LinearDepthPyramid(DxCommandList* cl,
    ref<DxTexture> depthStencilBuffer,
    ref<DxTexture> linearDepthBuffer,
    vec4 projectionParams);