#include "DepthOnly_rs.hlsli"

ConstantBuffer<DepthOnlyObjectIdCB> id : register(b1);
ConstantBuffer<DepthOnlyCameraJitterCB> jitter : register(b2);

struct PsInput
{
    float3 ndc : NDC;
    float3 prevFrameNdc : PREV_FRAME_NDC;
};
struct PsOutput
{
    float2 screenVelocity : SV_Target0;
    uint objectID : SV_Target1;
};

PsOutput main(PsInput psIn)
{
    //  divide homogeneous w actually; subtract camera jitter
    float2 ndc = (psIn.ndc.xy / psIn.ndc.z) - jitter.jitter;
    float2 prevNdc = (psIn.prevFrameNdc.xy / psIn.prevFrameNdc.z) - jitter.preFrameJitter;
    float2 motion = (prevNdc - ndc) * float2(0.5f, -0.5f);      //y axis opposite in screen space
    PsOutput psOut;
    psOut.screenVelocity = motion;
    psOut.objectID = id.id;
    return psOut;
}