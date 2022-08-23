#include "sky_rs.hlsli"

ConstantBuffer<SKYCB> skyIntensity : register(b1);
SamplerState texSampler : register(s0);
TextureCube<float4> textureCube : register(t0);

struct PsInput
{
    float3 uv : TEXCOORDS;
};

struct PsOutput
{
    float4 color : SV_Target0;
    float2 screenVelocity : SV_Target1;
    uint objectID : SV_Target2;
};

[RootSignature(SKY_TEXTURE_RS)]
PsOutput main(PsInput psIn)
{
    PsOutput psOut;
    psOut.color = float4( (textureCube.Sample(texSampler, psIn.uv) * skyIntensity.intensity).xyz, 0.f);
    //TODO: sjw.
    psOut.screenVelocity = float2(0.f, 0.f);
    psOut.objectID = 0xFFFFFFFF;
    return psOut;
}