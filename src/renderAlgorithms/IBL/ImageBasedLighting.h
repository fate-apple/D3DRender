#pragma once
#include "pch.h"
#include "core/reflect.h"

struct SSR_Settings
{
    uint32 numSteps = 400;              //no sdf
    float maxDistance = 1000.f;
    float strideCutoff = 100.f;
    float minStride = 5.f;
    float maxStride = 30.f;
};
REFLECT_STRUCT(SSR_Settings,
    (numSteps, "Num steps"),
    (maxDistance, "Max distance"),
    (strideCutoff, "Stride cutoff"),
    (minStride, "Min stride"),
    (maxStride, "Max stride")
);

struct SSS_Settings
{
    uint32 numSteaps = 16;
    float rayDistance = 0.5f;
    float thickness = 0.05f;
    float maxDistanceFromCamera = 15.f;
    float distanceFadeoutRange = 2.f;
    float borderFadeout = 0.1f;
};
REFLECT_STRUCT(SSS_Settings, (numSteaps), (rayDistance), (thickness), (maxDistanceFromCamera), (distanceFadeoutRange), (borderFadeout));