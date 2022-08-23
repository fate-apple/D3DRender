#include "DepthOnly_rs.hlsli"

ConstantBuffer<DepthOnlyTransformCB> transform : register(b0);

struct MeshPosition
{
    float3 position;
};
StructuredBuffer<MeshPosition> prevFramePositions : register(t0);

struct VsInput
{
    float3 position : POSITION;
    uint vertexId   : SV_VertexID;
};
struct VsOutput
{
    float4 position : SV_POSITION;
    float3 ndc : NDC;
    float3 prevFrameNDC: PREV_FRAME_NDC;
};

[RootSignature(ANIMATED_DEPTH_ONLY_RS)]
VsOutput main(VsInput vsIn)
{
    VsOutput vsOut;
    vsOut.position = mul(transform.mvp, float4(vsIn.position, 1.f));
    vsOut.ndc = vsOut.position.xyw;
    float3 prevFramePosition = prevFramePositions[vsIn.vertexId].position;
    vsOut.prevFrameNDC = mul( transform.prevFrameMVP, float4(prevFramePosition, 1.f)).xyw;
    return vsOut;
}
