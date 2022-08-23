#include  "Cs.hlsli"
#include "Skinning_rs.hlsli"
//Games 204 chapter 7,8,9

#define BLOCK_SIZE 512

ConstantBuffer<SkinningCB> skinningCB : register(b0);
StructuredBuffer<SkinnedMeshPosition> meshPositions : register(t0);
StructuredBuffer<SkinnedMeshOtherInfo> meshOtherInfos : register(t1);
StructuredBuffer<float4x4> meshMatrices :register(t2);

RWStructuredBuffer<MeshPosition> outputPositions : register(u0);
RWStructuredBuffer<MeshOtherInfo> outputOthers : register(u1);

[RootSignature(SKINNING_RS)]
[numthreads(BLOCK_SIZE, 1, 1)]
void main(CsInput csIn)
{
    uint index = csIn.dispatchThreadID.x;
    if(index > skinningCB.numVertices) {
        return;
    }

    float3 position = meshPositions[skinningCB.firstVertex + index].position;
    SkinnedMeshOtherInfo otherInfo = meshOtherInfos[skinningCB.firstVertex + index];

    uint4 skinIndices = uint4(
        otherInfo.skinIndices >> 24,
        otherInfo.skinIndices >> 16 & 0xff,
        otherInfo.skinIndices >> 8 & 0xff,
        otherInfo.skinIndices & 0xff);
    float4 skinWeights = float4(
        otherInfo.skinWeights >> 24,
        otherInfo.skinWeights >> 16 & 0xff,
        otherInfo.skinWeights >> 8 & 0xff,
        otherInfo.skinWeights & 0xff) * (1.f / 255.f);
    skinWeights /= dot(skinWeights, float4(1.f, 1.f, 1.f, 1.f));
    skinIndices += skinningCB.firstJoint.xxxx;

    float4x4 m = meshMatrices[skinIndices.x] * skinWeights.x +
    meshMatrices[skinIndices.y] * skinWeights.y +
    meshMatrices[skinIndices.z] * skinWeights.z +
    meshMatrices[skinIndices.w] * skinWeights.w;

    //TODO: sjw. read unreal
    float3 outPosition = mul(m, float4(position, 1.f)).xyz;
    float3 outNormal = mul(m, float4(otherInfo.normal, 1.f)).xyz;
    float3 outTangent = mul(m, float4(otherInfo.tangent, 1.f)).xyz;

    MeshPosition outmMeshPosition  = {outPosition};
    MeshOtherInfo outMeshOtherInfo = {outNormal, outTangent, otherInfo.uv};
    outputPositions[skinningCB.writeOffset + index] = outmMeshPosition;
    outputOthers[skinningCB.writeOffset + index] = outMeshOtherInfo;
}