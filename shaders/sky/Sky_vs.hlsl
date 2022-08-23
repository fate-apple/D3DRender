#include "sky_rs.hlsli"

ConstantBuffer<SkyTransformCB> sky : register(b0);

struct VsInput
{
    uint vertexID : SV_VertexID;
};
struct VSOutput
{
    float3 uv : TEXCOORDS;
    float4 position : SV_Position;
};

VSOutput main(VsInput vsIn)
{
    VSOutput vsOut;
    uint v = 1 << vsIn.vertexID;
    //Game Engine Gems3 Chapter 6 table6.1. 14 point
    float3 uv = float3(
        (0x287a & v) != 0, 
        (0x02af & v) != 0, 
        (0x31e3 & v) != 0
    ) * 2.f - 1.f;
    vsOut.uv = uv;
    vsOut.position = mul(sky.viewProject, float4(uv, 1.f));
    vsOut.position.z = vsOut.position.w - 1e-6f;
    return vsOut;
}