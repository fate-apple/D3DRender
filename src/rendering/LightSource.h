#pragma once
#include "LightSource.hlsli"
#include "core/Camera.h"
#include "core/Math.h"
#include "core/reflect.h"


// light info which should keep in gpu defined in lightsource.hlsli;
// here is light source info in scene. don't care about alignment
struct DirectionalLight
{
    vec3 color;
    float intensity;
    vec3 direction;
    
    uint32 numShadowCascades;
    vec4 cascadeDistances;
    vec4 bias;

    //TODO: sjw. we assume only use directionLight as sun for now;
    vec4 shadowMapViewports[MAX_NUM_SUN_SHADOW_CASCADES];
    mat4 viewProjs[MAX_NUM_SUN_SHADOW_CASCADES];

    vec4 blendDistances;
    uint32 shadowDimensions = 2048;
    bool stabilize;

    void UpdateMatrices(const RenderCamera& camera);
};
REFLECT_STRUCT(DirectionalLight,
    (color, "Color"),
    (intensity, "Intensity"),
    (direction, "Direction"),
    (numShadowCascades, "Cascades"),
    (cascadeDistances, "Cascade distances"),
    (bias, "Bias"),
    (blendDistances, "Blend distances"),
    (shadowDimensions, "Shadow dimensions"),
    (stabilize, "Stabilize")
);