#include "Cs.hlsli"
#include "Camera.hlsli"
#include "LightCulling_rs.hlsli"

#define BLOCK_SIZE 16

ConstantBuffer<CameraCB> camera	: register(b0);
ConstantBuffer<FrustaCB> cb	    : register(b1);     //{ screenWidth / tileWidth, screenHeight / tileHeight}
RWStructuredBuffer<LightCullingViewFrustum> outFrusta  : register(u0);


static LightCullingViewFrustumPlane GetPlane(float3 p0, float3 p1, float3 p2)       //right hand rule in principle, to make The plane normal points to the inside of the frustum.
{
    LightCullingViewFrustumPlane result;

    float3 v0 = p1 - p0;
    float3 v2 = p2 - p0;

    result.N = normalize(cross(v0, v2));
    result.d = dot(result.N, p0);

    return result;
}

// one thread one tile
[RootSignature(WORLD_SPACE_TILED_FRUSTA_RS)]
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void main(CsInput csIn)
{
    float2 invScreenDims = camera.invScreenDims;

    float2 screenTL = invScreenDims * (csIn.dispatchThreadID.xy * LIGHT_CULLING_TILE_SIZE);
    float2 screenTR = invScreenDims * (float2(csIn.dispatchThreadID.x + 1, csIn.dispatchThreadID.y) * LIGHT_CULLING_TILE_SIZE);
    float2 screenBL = invScreenDims * (float2(csIn.dispatchThreadID.x, csIn.dispatchThreadID.y + 1) * LIGHT_CULLING_TILE_SIZE);
    float2 screenBR = invScreenDims * (float2(csIn.dispatchThreadID.x + 1, csIn.dispatchThreadID.y + 1) * LIGHT_CULLING_TILE_SIZE);

    // Points on near plane.
    float3 tl = camera.RestoreWorldSpacePosition(screenTL, 0.f);
    float3 tr = camera.RestoreWorldSpacePosition(screenTR, 0.f);
    float3 bl = camera.RestoreWorldSpacePosition(screenBL, 0.f);
    float3 br = camera.RestoreWorldSpacePosition(screenBR, 0.f);

    float3 cameraPos = camera.position.xyz;
    LightCullingViewFrustum frustum;
    frustum.planes[0] = GetPlane(cameraPos, bl, tl);        //right hand
    frustum.planes[1] = GetPlane(cameraPos, tr, br);
    frustum.planes[2] = GetPlane(cameraPos, tl, tr);
    frustum.planes[3] = GetPlane(cameraPos, br, bl);
    //one thread per tile
    if (csIn.dispatchThreadID.x < cb.numThreadsX && csIn.dispatchThreadID.y < cb.numThreadsY)
    {
        uint index = csIn.dispatchThreadID.y * cb.numThreadsX + csIn.dispatchThreadID.x;
        //from top-left to bottom-right. eg. same order with screen
        outFrusta[index] = frustum;
    }
}
