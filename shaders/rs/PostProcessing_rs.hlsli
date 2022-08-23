#ifndef POST_PROCESSING_RS_HLSLI
#define POST_PROCESSING_RS_HLSLI


#define POST_PROCESSING_BLOCK_SIZE 16

struct HierarchicalLinearDepthCB
{
    vec4 projectionParams;
    vec2 invDimensions;
};

#define HIERARCHICAL_LINEAR_DEPTH_RS \
"RootFlags(0), " \
"RootConstants(b0, num32BitConstants = 8), " \
"DescriptorTable( UAV(u0, numDescriptors = 6), SRV(t0, numDescriptors = 1) )," \
"StaticSampler(s0," \
"addressU = TEXTURE_ADDRESS_CLAMP," \
"addressV = TEXTURE_ADDRESS_CLAMP," \
"addressW = TEXTURE_ADDRESS_CLAMP," \
"filter = FILTER_MIN_MAG_MIP_LINEAR)"


#define HIERARCHICAL_LINEAR_DEPTH_RS_CB           0
#define HIERARCHICAL_LINEAR_DEPTH_RS_TEXTURES     1



// ----------------------------------------
// TEMPORAL ANTI-ALIASING
// ----------------------------------------

struct TAA_CB
{
    vec4 projectionParams;
    vec2 dimensions;
};

#define TAA_RS \
"RootFlags(0), " \
"RootConstants(num32BitConstants=8, b0),"  \
"DescriptorTable( UAV(u0, numDescriptors = 1), SRV(t0, numDescriptors = 4) ), " \
"StaticSampler(s0," \
"addressU = TEXTURE_ADDRESS_CLAMP," \
"addressV = TEXTURE_ADDRESS_CLAMP," \
"addressW = TEXTURE_ADDRESS_CLAMP," \
"filter = FILTER_MIN_MAG_MIP_LINEAR)"

#define TAA_RS_CB               0
#define TAA_RS_TEXTURES         1


// ----------------------------------------
// HBAO
// ----------------------------------------

struct HBAO_CB
{
    uint32 screenWidth;
    uint32 screenHeight;
    float depthBufferMipLevel;
    uint32 numRays;
    vec2 rayDeltaRotation;
    uint32 maxNumStepsPerRay;
    float radius;
    float strength;
    float seed;
};

#define HBAO_RS \
"RootFlags(0), " \
"RootConstants(num32BitConstants=10, b0),"  \
"CBV(b1), " \
"DescriptorTable( UAV(u0, numDescriptors = 1), SRV(t0, numDescriptors = 1) ), " \
"StaticSampler(s0," \
"addressU = TEXTURE_ADDRESS_CLAMP," \
"addressV = TEXTURE_ADDRESS_CLAMP," \
"addressW = TEXTURE_ADDRESS_CLAMP," \
"filter = FILTER_MIN_MAG_MIP_LINEAR)"

#define HBAO_RS_CB                  0
#define HBAO_RS_CAMERA              1
#define HBAO_RS_TEXTURES            2

// ----------------------------------------
// SHADOW BLUR
// ----------------------------------------

struct ShadowBlurCB
{
    vec2 dimensions;
    vec2 invDimensions;
};

#define SHADOW_BLUR_X_RS \
"RootFlags(0), " \
"RootConstants(num32BitConstants=4, b0),"  \
"DescriptorTable( UAV(u0, numDescriptors = 1), SRV(t0, numDescriptors = 2) ), " \
"StaticSampler(s0," \
"addressU = TEXTURE_ADDRESS_CLAMP," \
"addressV = TEXTURE_ADDRESS_CLAMP," \
"addressW = TEXTURE_ADDRESS_CLAMP," \
"filter = FILTER_MIN_MAG_MIP_POINT), " \
"StaticSampler(s1," \
"addressU = TEXTURE_ADDRESS_CLAMP," \
"addressV = TEXTURE_ADDRESS_CLAMP," \
"addressW = TEXTURE_ADDRESS_CLAMP," \
"filter = FILTER_MIN_MAG_MIP_LINEAR)"

#define SHADOW_BLUR_Y_RS \
"RootFlags(0), " \
"RootConstants(num32BitConstants=4, b0),"  \
"DescriptorTable( UAV(u0, numDescriptors = 1), SRV(t0, numDescriptors = 4) ), " \
"StaticSampler(s0," \
"addressU = TEXTURE_ADDRESS_CLAMP," \
"addressV = TEXTURE_ADDRESS_CLAMP," \
"addressW = TEXTURE_ADDRESS_CLAMP," \
"filter = FILTER_MIN_MAG_MIP_POINT), " \
"StaticSampler(s1," \
"addressU = TEXTURE_ADDRESS_CLAMP," \
"addressV = TEXTURE_ADDRESS_CLAMP," \
"addressW = TEXTURE_ADDRESS_CLAMP," \
"filter = FILTER_MIN_MAG_MIP_LINEAR)"
#endif