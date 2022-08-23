#include "math.hlsli"
#include "Sky_rs.hlsli"

ConstantBuffer<SKYCB> skyIntensity : register(b1);

struct PsInput
{
    float3 uv : TEXCOORDS;
};
struct PsOutput
{
    float4 color : SV_Target0;
    float2 screenVelocity : SV_Target1;
    uint objectID : SV_target2;
};

[RootSignature(SKY_PROCEDURAL_RS)]
PsOutput main(PsInput psIn)
{
    float3 dir = normalize(psIn.uv);
    float2 panoUv = float2( atan2(-dir.x, -dir.z), acos(dir.y)) * float2(M_INV_PI, M_INV_PI);

    float step = 30.f;
    int x = (int)(panoUv.x * step) & 1;
    int y = (int)(panoUv.y * step) & 1;
    float i = remap((float)(x == y), 0.f, 1.f, 0.1f, 1.f) * skyIntensity.intensity;

    PsOutput psOut;
    psOut.color = float4(i * float3(0.4f, 0.6f, 0.2f), 0.f);
    psOut.screenVelocity = float2(0.f, 0.f);
    psOut.objectID  = 0xFFFFFFFF;
    return psOut;
    
}