#include "pch.h"
#include "Application.h"

#include "Material.hlsli"
#include "dx/DxContext.h"
#include "geometry/mesh.h"
#include "renderAlgorithms/raytracing/PbrRaytracer.h"
#include "renderAlgorithms/raytracing/Raytracing.h"
#include "scene/Scene.h"


static RaytracingObjectType defineBlasFromMesh(const ref<composite_mesh>& mesh)
{
    if (dxContext.featureSupport.Raytracing())
    {
        RaytracingBlasBuilder blasBuilder;
        std::vector<ref<PbrMaterial>> raytracingMaterials;

        for (auto& sm : mesh->submeshes)
        {
            blasBuilder.Push(mesh->mesh.vertexBuffer, mesh->mesh.indexBuffer, sm.info);
            raytracingMaterials.push_back(sm.material);
        }

        ref<RaytracingBlas> blas = blasBuilder.Finish();
        RaytracingObjectType type = PbrRaytracer::DefineObjectType(blas, raytracingMaterials);
        return type;
    }
    else
    {
        return {};
    }
}

void Application::Initialize(MainRenderer* inRenderer)
{
    this->renderer = inRenderer;
    if(dxContext.featureSupport.Raytracing()) {
        raytracingTLAS.Initialize();
    }
    game_scene& scene = this->scene.GetCurrentScene();
    scene.camera.InitializeIngame(vec3(0.f, 1.f, 0.f), quat::identity, deg2rad(70.f), 0.1f);
    editor.initialize(&this->scene, renderer);

    if (auto mesh = loadMeshFromFile("assets/geometries/room.obj"))
    {
	
        auto blas = defineBlasFromMesh(mesh);
	
        scene.createEntity("Sponza")
            .addComponent<transform_component>(vec3(10.f, 5.f, 100.f), quat::identity, 0.1f)
            .addComponent<raster_component>(mesh)
            .addComponent<raytrace_component>(blas);
    }

    editor.setEnvironment("assets/sky/2009 Dusk Pink Sky/2009 Dusk Pink Sky.hdr");
    RandomNumbterGenerator rng = { 14878213 };
    lightProbeGrid.initialize(vec3(-20.f, -1.f, -20.f), vec3(40.f, 20.f, 40.f), 1.5f);

    scene.sun.direction = normalize(vec3(-0.6f, -1.f, -0.3f));
    scene.sun.color = vec3(1.f, 0.93f, 0.76f);
    scene.sun.intensity = 50.f;

    scene.sun.numShadowCascades = 3;
    scene.sun.shadowDimensions = 2048;
    scene.sun.cascadeDistances = vec4(9.f, 25.f, 50.f, 10000.f);
    scene.sun.bias = vec4(0.000588f, 0.000784f, 0.000824f, 0.0035f);
    scene.sun.blendDistances = vec4(5.f, 10.f, 10.f, 10.f);
    scene.sun.stabilize = true;

    for (uint32 i = 0; i < NUM_BUFFERED_FRAMES; ++i)
    {
        pointLightBuffer[i] = CreateUploadBuffer(sizeof(PointLightCB), 512, 0);
        spotLightBuffer[i] = CreateUploadBuffer(sizeof(SpotLightCB), 512, 0);
        decalBuffer[i] = CreateUploadBuffer(sizeof(PbrDecalCB), 512, 0);

        spotLightShadowInfoBuffer[i] = CreateUploadBuffer(sizeof(SpotShadowInfo), 512, 0);
        pointLightShadowInfoBuffer[i] = CreateUploadBuffer(sizeof(PointShadowInfo), 512, 0);

        SET_NAME(pointLightBuffer[i]->resource, "Point lights");
        SET_NAME(spotLightBuffer[i]->resource, "Spot lights");
        SET_NAME(decalBuffer[i]->resource, "Decals");

        SET_NAME(spotLightShadowInfoBuffer[i]->resource, "Spot light shadow infos");
        SET_NAME(pointLightShadowInfoBuffer[i]->resource, "Point light shadow infos");
    }
    stackArena.Initialize();
}


void Application::SubmitRendererParams(uint32 numSpotLightShadowPasses, uint32 numPointLightShadowPasses)
{
    _opaqueRenderPass.Sort();
    _transParentPass.Sort();
    _ldrRenderPass.Sort();

    renderer->SubmitRenderPass(&_opaqueRenderPass);
    renderer->SubmitRenderPass(&_transParentPass);
    renderer->SubmitRenderPass(&_ldrRenderPass);

    MainRenderer::SubmitShadowRenderPass(&sunShadowRenderPass);
    for (uint32 i = 0; i < numSpotLightShadowPasses; ++i)
    {
        MainRenderer::SubmitShadowRenderPass(&spotShadowRenderPasses[i]);
    }

    for (uint32 i = 0; i < numPointLightShadowPasses; ++i)
    {
        MainRenderer::SubmitShadowRenderPass(&pointShadowRenderPasses[i]);
    }
    MainRenderer::RaytraceLightProbes(lightProbeGrid);
}

