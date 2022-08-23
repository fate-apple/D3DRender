#ifndef LIGHT_SOURCE_H
#define LIGHT_SOURCE_H

#include "shadow/PCF.hlsli"


#define MAX_NUM_SUN_SHADOW_CASCADES 4
#define LIGHT_RADIANCE_SCALE 1000.f

struct DirectionalLightCB
{
    mat4 viewProjs[MAX_NUM_SUN_SHADOW_CASCADES];
    vec4 viewports[MAX_NUM_SUN_SHADOW_CASCADES];
    vec4 cascadeDistances;
    vec4 bias;

    vec4 blendDistances;
    vec3 direction;
    uint32 numShadowCascades;
    vec3 radiance;
    uint32 padding;
};

struct PointLightCB
{
    vec3 position;
    float radius; // Maximum distance.
    vec3 radiance;
    int shadowInfoIndex; // -1, if light casts no shadows.


    void initialize(vec3 position_, vec3 radiance_, float radius_, int shadowInfoIndex_ = -1)
    {
        position = position_;
        radiance = radiance_;
        radius = radius_;
        shadowInfoIndex = shadowInfoIndex_;
    }

#ifndef HLSL
    PointLightCB() {}

    PointLightCB(vec3 position_, vec3 radiance_, float radius_, int shadowInfoIndex_ = -1)
    {
        initialize(position_, radiance_, radius_, shadowInfoIndex_);
    }
#endif
};


struct SpotLightCB
{
    vec3 position;
    //attenuation start from inner angle as 1 to  Outer angle as 0
    int innerAndOuterCutoff; // cos(innerAngle) << 16 | cos(outerAngle). Both are packed into 16 bit signed ints.
    vec3 direction;
    float maxDistance;
    vec3 radiance;
    int shadowInfoIndex; // -1, if light casts no shadows.


    void initialize(vec3 position_, vec3 direction_, vec3 radiance_, float innerAngle_, float outerAngle_, float maxDistance_,
                    int shadowInfoIndex_ = -1)
    {
        position = position_;
        direction = direction_;
        radiance = radiance_;

        int inner = (int)(cos(innerAngle_) * ((1 << 15) - 1));
        int outer = (int)(cos(outerAngle_) * ((1 << 15) - 1));
        innerAndOuterCutoff = (inner << 16) | outer;

        maxDistance = maxDistance_;
        shadowInfoIndex = shadowInfoIndex_;
    }

#ifndef HLSL
    spot_light_cb() {}

    spot_light_cb(vec3 position_, vec3 direction_, vec3 radiance_, float innerAngle_, float outerAngle_, float maxDistance_, int shadowInfoIndex_ = -1)
    {
        initialize(position_, direction_, radiance_, innerAngle_, outerAngle_, maxDistance_, shadowInfoIndex_);
    }
#endif

    float getInnerCutoff()
#ifndef HLSL
        const
#endif
    {
        return (innerAndOuterCutoff >> 16) / float((1 << 15) - 1);
    }

    float getOuterCutoff()
#ifndef HLSL
        const
#endif
    {
        return (innerAndOuterCutoff & 0xFFFF) / float((1 << 15) - 1);
    }
};


struct SpotShadowInfo
{
    mat4 viewProj;

    vec4 viewport;

    float bias;
    float padding0[3];
};

struct PointShadowInfo
{
    vec4 viewport0;
    vec4 viewport1;
};

// https://imdoingitwrong.wordpress.com/2011/02/10/improved-light-attenuation/
static float getAttenuation(float distance, float maxDistance)
{
    float relDist = min(distance / maxDistance, 1.f);
    float d = distance / (1.f - relDist * relDist);

    float att = 1.f / (d * d + 1.f);
    return att;
}


#endif