#include "DefaultPbr_rs.hlsli"

ConstantBuffer<TransformCB> transform : register(b0);

struct VsInput
{
    float3 position     : POSITION;
    float3 normal       : NORMAL;
    float3 tangent      : TANGENT;
    float2 uv           : TEXCOORDS;
};

struct VsOutput
{
    float4 position         : SV_POSITION;
    float3 worldPosition    : POSITION;
    float3x3 tbn            : TANGENT_FRAME;
    float2 uv               : TEXCOORDS;
};

VsOutput main(VsInput vsIn)
{
    VsOutput OUT;
    OUT.position = mul(transform.mvp, float4(vsIn.position, 1.f));
    OUT.worldPosition = mul(transform.m, float4(vsIn.position, 1.f));
    float3 normal = normalize(mul(transform.m, float4(vsIn.normal, 0.f)).xyz);
    float3 tangent = normalize(mul(transform.m, float4(vsIn.tangent, 0.f)).xyz);
    float3 biTangent = normalize(cross(normal, tangent));       // n => z, t => x.
    OUT.tbn = float3x3(tangent, biTangent, normal);
    OUT.uv = vsIn.uv;

    return OUT;
}