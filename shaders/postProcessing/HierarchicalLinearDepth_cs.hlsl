#include "Cs.hlsli"
#include "PostProcessing_rs.hlsli"
#include "Camera.hlsli"

ConstantBuffer<HierarchicalLinearDepthCB> cb : register(b0);

//Tex2D(linearDepthFormat, renderWidth, renderHeight, 1, 6, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
RWTexture2D<float> outputMip0 : register(u0);
RWTexture2D<float> outputMip1 : register(u1);
RWTexture2D<float> outputMip2 : register(u2);
RWTexture2D<float> outputMip3 : register(u3);
RWTexture2D<float> outputMip4 : register(u4);
RWTexture2D<float> outputMip5 : register(u5);

Texture2D<float> input : register(t0);
SamplerState linearClampSampler : register(s0);

groupshared float tile[POST_PROCESSING_BLOCK_SIZE][POST_PROCESSING_BLOCK_SIZE];

// one thread handle -> four pixels in depthBuffer
[RootSignature(HIERARCHICAL_LINEAR_DEPTH_RS)]
[numthreads(POST_PROCESSING_BLOCK_SIZE, POST_PROCESSING_BLOCK_SIZE, 1)]
void main(CsInput In)
{
    const uint2 groupThreadID = In.groupThreadID.xy;
    const uint2 dispatchThreadID = In.dispatchThreadID.xy;
    // 4 pixels
    float4 depths = float4(
        input[dispatchThreadID * 2 + uint2(0, 0)],
        input[dispatchThreadID * 2 + uint2(1, 0)],
        input[dispatchThreadID * 2 + uint2(0, 1)],
        input[dispatchThreadID * 2 + uint2(1, 1)]
    );
    float maxdepth = max(depths.x, max(depths.y, max(depths.z, depths.w)));
    tile[groupThreadID.x][groupThreadID.y] = maxdepth;
    GroupMemoryBarrierWithGroupSync();

    if(groupThreadID.x % 2 == 0 && groupThreadID.y % 2 == 0) {
        maxdepth = max(tile[groupThreadID.x][groupThreadID.y],
                       max(tile[groupThreadID.x + 1][groupThreadID.y],
                           max(tile[groupThreadID.x][groupThreadID.y + 1],
                               tile[groupThreadID.x + 1][groupThreadID.y + 1])));
        tile[groupThreadID.x][groupThreadID.y] = maxdepth;
    }
    GroupMemoryBarrierWithGroupSync(); //16 pixels
    // 4 pixels in camera space
    const float4 lineardepths = float4
    (
        DepthBufferDepthToEyeDepth(depths.x, cb.projectionParams),
        DepthBufferDepthToEyeDepth(depths.y, cb.projectionParams),
        DepthBufferDepthToEyeDepth(depths.z, cb.projectionParams),
        DepthBufferDepthToEyeDepth(depths.w, cb.projectionParams)
    );
    outputMip0[dispatchThreadID * 2 + uint2(0, 0)] = lineardepths.x;
    outputMip0[dispatchThreadID * 2 + uint2(1, 0)] = lineardepths.y;
    outputMip0[dispatchThreadID * 2 + uint2(0, 1)] = lineardepths.z;
    outputMip0[dispatchThreadID * 2 + uint2(1, 1)] = lineardepths.w;

    maxdepth = max(lineardepths.x, max(lineardepths.y, max(lineardepths.z, lineardepths.w)));       // recall project matrix m32 = -1, so: far >= linearDepth >= near > 0
    tile[groupThreadID.x][groupThreadID.y] = maxdepth;
    outputMip1[dispatchThreadID] = maxdepth;
    GroupMemoryBarrierWithGroupSync();

    if (groupThreadID.x % 2 == 0 && groupThreadID.y % 2 == 0)
    {
        maxdepth = max(tile[groupThreadID.x][groupThreadID.y], max(tile[groupThreadID.x + 1][groupThreadID.y], max(tile[groupThreadID.x][groupThreadID.y + 1], tile[groupThreadID.x + 1][groupThreadID.y + 1])));
        tile[groupThreadID.x][groupThreadID.y] = maxdepth;
        outputMip2[dispatchThreadID / 2] = maxdepth;        //4*4
    }
    GroupMemoryBarrierWithGroupSync();

    if (groupThreadID.x % 4 == 0 && groupThreadID.y % 4 == 0)
    {
        maxdepth = max(tile[groupThreadID.x][groupThreadID.y], max(tile[groupThreadID.x + 2][groupThreadID.y], max(tile[groupThreadID.x][groupThreadID.y + 2], tile[groupThreadID.x + 2][groupThreadID.y + 2])));
        tile[groupThreadID.x][groupThreadID.y] = maxdepth;
        outputMip3[dispatchThreadID / 4] = maxdepth;
    }
    GroupMemoryBarrierWithGroupSync();

    if (groupThreadID.x % 8 == 0 && groupThreadID.y % 8 == 0)
    {
        maxdepth = max(tile[groupThreadID.x][groupThreadID.y], max(tile[groupThreadID.x + 4][groupThreadID.y], max(tile[groupThreadID.x][groupThreadID.y + 4], tile[groupThreadID.x + 4][groupThreadID.y + 4])));
        tile[groupThreadID.x][groupThreadID.y] = maxdepth;
        outputMip4[dispatchThreadID / 8] = maxdepth;
    }
    GroupMemoryBarrierWithGroupSync();

    if (groupThreadID.x % 16 == 0 && groupThreadID.y % 16 == 0)
    {
        maxdepth = max(tile[groupThreadID.x][groupThreadID.y], max(tile[groupThreadID.x + 8][groupThreadID.y], max(tile[groupThreadID.x][groupThreadID.y + 8], tile[groupThreadID.x + 8][groupThreadID.y + 8])));
        outputMip5[dispatchThreadID / 16] = maxdepth;       //8*8 piexels
    }
}