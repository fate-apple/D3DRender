#ifndef SSR_RS_HLSLI
#define SSR_RS_HLSLI


#define SSR_BLOCK_SIZE 16
#define SSR_GGX_IMPORTANCE_SAMPLE_BIAS 0.1f
struct SSR_RaycastCB
{
    vec2 dimensions;
    vec2 invDimensions;
    uint32 frameIndex;
    uint32 numSteps;

    float maxDistance;
    float strideCutOff;
    float minStride;
    float maxStride;
};


#define SSR_RAYCAST_RS \
"RootFlags(0), " \
"RootConstants(b0, num32BitConstants = 10), " \
"CBV(b1), " \
"DescriptorTable( UAV(u0, numDescriptors = 1), SRV(t0, numDescriptors = 5) )," \
"StaticSampler(s0," \
"addressU = TEXTURE_ADDRESS_CLAMP," \
"addressV = TEXTURE_ADDRESS_CLAMP," \
"addressW = TEXTURE_ADDRESS_CLAMP," \
"filter = FILTER_MIN_MAG_MIP_LINEAR), " \
"StaticSampler(s1," \
"addressU = TEXTURE_ADDRESS_CLAMP," \
"addressV = TEXTURE_ADDRESS_CLAMP," \
"addressW = TEXTURE_ADDRESS_CLAMP," \
"filter = FILTER_MIN_MAG_MIP_POINT)"

#define SSR_RAYCAST_RS_CB           0
#define SSR_RAYCAST_RS_CAMERA       1
#define SSR_RAYCAST_RS_TEXTURES     2

struct SSR_ResolveCB
{
    vec2 dimensions;
    vec2 invDimensions;
};

#define SSR_RESOLVE_RS \
"RootFlags(0), " \
"RootConstants(b0, num32BitConstants = 4), " \
"CBV(b1), " \
"DescriptorTable( UAV(u0, numDescriptors = 1), SRV(t0, numDescriptors = 5) )," \
"StaticSampler(s0," \
"addressU = TEXTURE_ADDRESS_CLAMP," \
"addressV = TEXTURE_ADDRESS_CLAMP," \
"addressW = TEXTURE_ADDRESS_CLAMP," \
"filter = FILTER_MIN_MAG_MIP_LINEAR), " \
"StaticSampler(s1," \
"addressU = TEXTURE_ADDRESS_CLAMP," \
"addressV = TEXTURE_ADDRESS_CLAMP," \
"addressW = TEXTURE_ADDRESS_CLAMP," \
"filter = FILTER_MIN_MAG_MIP_POINT)"


#define SSR_RESOLVE_RS_CB           0
#define SSR_RESOLVE_RS_CAMERA       1
#define SSR_RESOLVE_RS_TEXTURES     2


struct SSR_TemporalCB
{
    vec2 invDimensions;
};

#define SSR_TEMPORAL_RS \
"RootFlags(0), " \
"RootConstants(b0, num32BitConstants = 2), " \
"DescriptorTable( UAV(u0, numDescriptors = 1), SRV(t0, numDescriptors = 3) )," \
"StaticSampler(s0," \
"addressU = TEXTURE_ADDRESS_CLAMP," \
"addressV = TEXTURE_ADDRESS_CLAMP," \
"addressW = TEXTURE_ADDRESS_CLAMP," \
"filter = FILTER_MIN_MAG_MIP_LINEAR)"


#define SSR_TEMPORAL_RS_CB           0
#define SSR_TEMPORAL_RS_TEXTURES     1

struct SSR_MedianBlurCB
{
    vec2 invDimensions;
};

#define SSR_MEDIAN_BLUR_RS \
"RootFlags(0), " \
"RootConstants(b0, num32BitConstants = 2), " \
"DescriptorTable( UAV(u0, numDescriptors = 1), SRV(t0, numDescriptors = 1) )," \
"StaticSampler(s0," \
"addressU = TEXTURE_ADDRESS_CLAMP," \
"addressV = TEXTURE_ADDRESS_CLAMP," \
"addressW = TEXTURE_ADDRESS_CLAMP," \
"filter = FILTER_MIN_MAG_MIP_LINEAR)"


#define SSR_MEDIAN_BLUR_RS_CB           0
#define SSR_MEDIAN_BLUR_RS_TEXTURES     1

#endif