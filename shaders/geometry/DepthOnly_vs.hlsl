#include "DepthOnly_rs.hlsli"

ConstantBuffer<DepthOnlyTransformCB> transform : register(b0);

struct VsInput
{
    float3 position : POSITION;
};
struct VsOutput
{
    float4 position : SV_POSITION;
    float3 ndc : NDC;
    float3 prevFrameNdc : PREV_FRAME_NDC;
};

[RootSignature(DEPTH_ONLY_RS)]
VsOutput main(VsInput vsIn)
{
    VsOutput vsOut;
    vsOut.position = mul(transform.mvp, float4(vsIn.position, 1.f));
    vsOut.ndc = vsOut.position.xyw;
    vsOut.prevFrameNdc = mul(transform.prevFrameMVP, float4(vsIn.position, 1.f)).xyw;
    return vsOut;
}