#include "Camera.hlsli"
#include "LightCulling_rs.hlsli"
#include "Cs.hlsli"
#include "LightSource.hlsli"
#include "Material.hlsli"

// https://wickedengine.net/2018/01/10/optimizing-tile-based-light-culling/
// Create two bitMask, one for tile and one for light which we are checking
// However, we don't take the advice to separate the big shader into several smaller parts, which is helpful to reduce register pressure, atomic operation and divergent branching.
// because we aimed to deal the scene which is not so complex and just for learning use.

ConstantBuffer<CameraCB> camera : register(b0);
ConstantBuffer<LightCullingCB> cb : register(b1);

Texture2D<float> depthBuffer : register(t0);
StructuredBuffer<LightCullingViewFrustum> frusta : register(t1);

StructuredBuffer<PointLightCB> pointLights : register(t2);
StructuredBuffer<SpotLightCB> spotLights : register(t3);
StructuredBuffer<PbrDecalCB> decals : register(t4);

RWTexture2D<uint4> tiledCullingGrid : register(u0);      //numCullingTilesX * numCullingTilesY. Keep offset of each tile in tiledObjectsIndexList 
RWStructuredBuffer<uint> tiledCullingIndexCounter : register(u1);
RWStructuredBuffer<uint> tiledObjectsIndexList : register(u2);      //groupLightCountOpaque + NUM_DECAL_BUCKETS + groupLightCountTransparent + NUM_DECAL_BUCKETS of each tile


groupshared uint groupMinDepth;
groupshared uint groupMaxDepth;
groupshared LightCullingViewFrustum groupFrustum;

groupshared float3 groupViewSpaceAABBCorners[8];
groupshared float3 groupViewSpaceAABBCenter;
groupshared float3 groupViewSpaceAABBExtent;

groupshared uint groupTileDepthMask;

groupshared uint groupObjectsStartOffset;

// Opaque.
groupshared uint groupObjectsListOpaque[MAX_NUM_INDICES_PER_TILE];          //lights (not culled) in this tile
groupshared uint groupLightCountOpaque;                                     //num of light (not culled) in this tile. gather in tiledCullingIndexCounter, we keep offset(spot start) info in tiledCullingGrid

// Transparent.
groupshared uint groupObjectsListTransparent[MAX_NUM_INDICES_PER_TILE];
groupshared uint groupLightCountTransparent;


// normal point to outside. so dot >0 indicates outside
static bool pointOutsidePlane(float3 p, LightCullingViewFrustumPlane plane)
{
    return dot(plane.N, p) - plane.d < 0;
}

static bool decalOutsidePlane(PbrDecalCB decal, LightCullingViewFrustumPlane plane)
{
    float x = dot(decal.right, plane.N) >= 0.f ? 1.f : -1.f;
    float y = dot(decal.up, plane.N) >= 0.f ? 1.f : -1.f;
    float z = dot(decal.forward, plane.N) >= 0.f ? 1.f : -1.f;

    float3 diag = x * decal.right + y * decal.up + z * decal.forward;

    float3 nPoint = decal.position + diag;
    return pointOutsidePlane(nPoint, plane);
}


static bool DecalInsideFrustum(PbrDecalCB decal, LightCullingViewFrustum frustum, LightCullingViewFrustumPlane nearPlane,
                               LightCullingViewFrustumPlane farPlane)
{
    bool result = true;

    if(decalOutsidePlane(decal, nearPlane)
        || decalOutsidePlane(decal, farPlane)) {
        result = false;
    }

    for(int i = 0; i < 4 && result; ++i) {
        if(decalOutsidePlane(decal, frustum.planes[i])) {
            result = false;
        }
    }

    return result;
}

// We flip the index, such that the first set bit corresponds to the front-most decal. We render the decals front to back, such that we can early exit when alpha >= 1.
// 8 bucket, uint(32 bit) indicate whether correspond decal is used. MAX_NUM_TOTAL_DECALS = 8 * 32 = 256
#define groupAppendDecal(type, index)                               \
{                                                               \
const uint v = MAX_NUM_TOTAL_DECALS - index - 1;            \
const uint bucket = v >> 5;                                 \
const uint bit = v & ((1 << 5) - 1);                        \
InterlockedOr(groupObjectsList##type[bucket], 1 << bit);    \
}

#define groupAppendLight(type, index)                               \
{                                                               \
    uint v;                                                     \
    InterlockedAdd(groupLightCount##type, 1, v);                \
    if (v < MAX_NUM_LIGHTS_PER_TILE)                            \
    {                                                           \
        groupObjectsList##type[TILE_LIGHT_OFFSET + v] = index;  \
    }                                                           \
}

#define groupAppendLightOpaque(index) groupAppendLight(Opaque, index)
#define groupAppendLightTransparent(index) groupAppendLight(Transparent, index)
#define groupAppendDecalOpaque(index) groupAppendDecal(Opaque, index)
#define groupAppendDecalTransparent(index) groupAppendDecal(Transparent, index)

struct SpotLightBoundingVolume
{
    float3 tip;
    float height;
    float3 direction;
    float radius;
};
static SpotLightBoundingVolume GetSpotLightBoundingVolume(SpotLightCB cb)
{
    SpotLightBoundingVolume result;
    result.tip = cb.position;
    result.direction = cb.direction;
    result.height = cb.maxDistance;
    float oc = cb.getOuterCutoff();
    result.radius = result.height * (1.f - oc * oc) / oc; //height * tan(outerAngle)
    return result;
}

static bool PointLightOutsidePlane(PointLightCB pl, LightCullingViewFrustumPlane plane)
{
    return dot(plane.N, pl.position) - plane.d < -pl.radius;
}

static bool SpotLightOutsidePlane(SpotLightBoundingVolume sl, LightCullingViewFrustumPlane plane)
{
    //just image normal as up. y is the projection of normal in circle plane (dot has max value)
    float3 y = normalize(cross(cross(plane.N, sl.direction), sl.direction));
    float3 Q = sl.tip + sl.direction * sl.height - y * sl.radius;
    return pointOutsidePlane(sl.tip, plane) && pointOutsidePlane(Q, plane);
}

static bool PointLightInsideFrustum(PointLightCB pl, LightCullingViewFrustum frustum, LightCullingViewFrustumPlane nearPlane,
                                    LightCullingViewFrustumPlane farPlane)
{
    bool result = true;

    if(PointLightOutsidePlane(pl, nearPlane)
        || PointLightOutsidePlane(pl, farPlane)) {
        result = false;
    }
    for(int i = 0; i < 4 && result; ++i) {
        if(PointLightOutsidePlane(pl, frustum.planes[i])) {
            result = false;
        }
    }
    return result;
}

static bool SpotLightInsideFrustum(SpotLightBoundingVolume sl, LightCullingViewFrustum frustum,
                                   LightCullingViewFrustumPlane nearPlane, LightCullingViewFrustumPlane farPlane)
{
    bool result = true;
    if(SpotLightOutsidePlane(sl, nearPlane)
        || SpotLightOutsidePlane(sl, farPlane)) {
        result = false;
    }
    for(int i = 0; i < 4 && result; ++i) {
        if(SpotLightOutsidePlane(sl, frustum.planes[i])) {
            result = false;
        }
    }
    return result;
}

//recall we first find the point at AABB which is nearest to sphere center, using Axis decomposition
static bool SphereInAABB(vec3 sphereCenter, float sphereRadius, float3 aabbCenter, float3 aabbExtents)
{
    float3 delta = max(0, abs(aabbCenter - sphereCenter) - aabbExtents);
    float distSq = dot(delta, delta);

    return distSq <= sphereRadius * sphereRadius;
}


static uint GetLightMask(float sphereCenterDepth, float radius, float depthRangeRecip, float nearZ)
{
    const float plMin = sphereCenterDepth - radius;
    const float plMax = sphereCenterDepth + radius;
    const uint plMaskIndexMin = max(0, min(31, floor((plMin - nearZ) * depthRangeRecip)));
    const uint plMaskIndexMax = max(0, min(31, floor((plMax - nearZ) * depthRangeRecip)));

    // Set all bits between plMaskIndexMin and (inclusive) plMaskIndexMax.
    uint lightMask = 0xFFFFFFFF;
    lightMask >>= 31 - (plMaskIndexMax - plMaskIndexMin);
    lightMask <<= plMaskIndexMin;
    return lightMask;
}

//one tile one group. one thread one pixel
[RootSignature(LIGHT_CULLING_RS)]
[numthreads(LIGHT_CULLING_TILE_SIZE, LIGHT_CULLING_TILE_SIZE, 1)]
void main(CsInput csIn)
{
    uint i;

    if(csIn.groupIndex == 0) {
        // Interprets the bit pattern of x as an unsigned integer.so that  store them in a texture
        // Since the depth is between 0 and 1, we can perform min and max operations on the bit pattern as uint. This is because native HLSL does not support floating point atomics.
        groupMinDepth = asuint(0.9999999f);
        groupMaxDepth = 0;
        groupLightCountOpaque = 0;
        groupLightCountTransparent = 0;

        groupTileDepthMask = 0;

        groupFrustum = frusta[csIn.groupID.y * cb.numThreadGroupsX + csIn.groupID.x];
    }

    // in this setting, groupObjectsListOpaque has up to 256 light and 8 decals. initialize first 8 Objects
    for(i = csIn.groupIndex; i < NUM_DECAL_BUCKETS; i += LIGHT_CULLING_TILE_SIZE * LIGHT_CULLING_TILE_SIZE) {
        groupObjectsListOpaque[i] = 0;
    }
    for(i = csIn.groupIndex; i < NUM_DECAL_BUCKETS; i += LIGHT_CULLING_TILE_SIZE * LIGHT_CULLING_TILE_SIZE) {
        groupObjectsListTransparent[i] = 0;
    }
    GroupMemoryBarrierWithGroupSync();

    uint2 screenSize;
    depthBuffer.GetDimensions(screenSize.x, screenSize.y);
    const float fDepth = depthBuffer[min(csIn.dispatchThreadID.xy, screenSize - 1)];
    if(fDepth != 1.f) {
        const uint uDepth = asuint(fDepth);

        InterlockedMin(groupMinDepth, uDepth);
        InterlockedMax(groupMaxDepth, uDepth);
    }
    GroupMemoryBarrierWithGroupSync();

    const float minDepth = asfloat(groupMinDepth);
    const float maxDepth = asfloat(groupMaxDepth);
    const float3 forward = camera.forward.xyz;
    const float3 cameraPos = camera.position.xyz;
    const float nearZ = camera.DepthBufferDepthToEyeDepth(minDepth);
    const float farZ = camera.DepthBufferDepthToEyeDepth(maxDepth);
    const float myZ = camera.DepthBufferDepthToEyeDepth(fDepth);
    //cluster 
    const float depthRangeRecip = 31.f / (farZ - nearZ);
    const uint depthMaskIndex = max(0, min(31, floor((myZ - nearZ) * depthRangeRecip)));
    InterlockedOr(groupTileDepthMask, 1 << depthMaskIndex);

    //nearest / farthest pixel in this tile
    const LightCullingViewFrustumPlane cameraNearPlane = {
        forward, dot(forward, cameraPos + camera.projectionParams.x * forward)
    };
    const LightCullingViewFrustumPlane nearPlane = {forward, dot(forward, cameraPos + nearZ * forward)};
    const LightCullingViewFrustumPlane farPlane = {-forward, -dot(forward, cameraPos + farZ * forward)};

    if(csIn.groupIndex < 8) {
        const uint x = csIn.groupIndex & 0x1;
        const uint y = (csIn.groupIndex >> 1) & 0x1;
        const uint z = (csIn.groupIndex >> 2);
        //eight corner coord in screenSpace
        const uint2 coord = (csIn.groupID.xy + uint2(x, y)) * LIGHT_CULLING_TILE_SIZE;
        const float2 uv = (float2(coord) + 0.5f) / screenSize; //pixel index to pixel center position

        groupViewSpaceAABBCorners[csIn.groupIndex] = camera.RevertViewSpacePosition(uv, lerp(minDepth, maxDepth, z));
    }
    GroupMemoryBarrierWithGroupSync();

    // always left-bottom-forward as minAABB? 
    if(csIn.groupIndex == 0) {
        float3 minAABB = 10000000;
        float3 maxAABB = -10000000;

        for(uint i = 0; i < 8; ++i) {
            minAABB = min(minAABB, groupViewSpaceAABBCorners[i]);
            maxAABB = max(maxAABB, groupViewSpaceAABBCorners[i]);
        }

        groupViewSpaceAABBCenter = (maxAABB + minAABB) * 0.5f;
        groupViewSpaceAABBExtent = maxAABB - groupViewSpaceAABBCenter;
    }
    GroupMemoryBarrierWithGroupSync();

    // Decals.
    const uint numDecals = cb.numDecals;
    for(i = csIn.groupIndex; i < numDecals; i += LIGHT_CULLING_TILE_SIZE * LIGHT_CULLING_TILE_SIZE) {
        PbrDecalCB d = decals[i];
        if(DecalInsideFrustum(d, groupFrustum, cameraNearPlane, farPlane)) {
            groupAppendDecalTransparent(i);

            if(!decalOutsidePlane(d, nearPlane)) {
                groupAppendDecalOpaque(i);
            }
        }
    }
    GroupMemoryBarrierWithGroupSync();

    // Point lights.
    // It is inefficient to utilize big thread group, because long iterating threads will hold back
    const uint numPointLights = cb.numPointLights;
    for(i = csIn.groupIndex; i < numPointLights; i += LIGHT_CULLING_TILE_SIZE * LIGHT_CULLING_TILE_SIZE) {
        PointLightCB pl = pointLights[i];
        if(PointLightInsideFrustum(pl, groupFrustum, cameraNearPlane, farPlane)) {
            groupAppendLightTransparent(i);

            pl.position = mul(camera.view, float4(pl.position, 1.f)).xyz;

            if(SphereInAABB(pl.position, pl.radius, groupViewSpaceAABBCenter, groupViewSpaceAABBExtent)) {
                const uint lightMask = GetLightMask(-pl.position.z, pl.radius, depthRangeRecip, nearZ);

                if(lightMask & groupTileDepthMask) {
                    groupAppendLightOpaque(i);
                }
            }
        }
    }
    GroupMemoryBarrierWithGroupSync();
    const uint numTilePointLightsOpaque = groupLightCountOpaque;
    const uint numTilePointLightsTransparent = groupLightCountTransparent;

    // Spot lights.
    const uint numSpotLights = cb.numSpotLights;
    for(i = csIn.groupIndex; i < numSpotLights; i += LIGHT_CULLING_TILE_SIZE * LIGHT_CULLING_TILE_SIZE) {
        const SpotLightCB sl = spotLights[i];
        const SpotLightBoundingVolume slBV = GetSpotLightBoundingVolume(sl);
        if(SpotLightInsideFrustum(slBV, groupFrustum, cameraNearPlane, farPlane)) {
            groupAppendLightTransparent(i);

            const float oc = sl.getOuterCutoff();
            //TODO: sjw. better culling strategy?
            //https://bartwronski.com/2017/04/13/cull-that-cone/
            const float radius = sl.maxDistance * 0.5f / (oc * oc);
            const float3 center = mul(camera.view, float4(sl.position + sl.direction * radius, 1.f)).xyz;

            if(SphereInAABB(center, radius, groupViewSpaceAABBCenter, groupViewSpaceAABBExtent)) {
                const uint lightMask = GetLightMask(-center.z, radius, depthRangeRecip, nearZ);

                if(lightMask & groupTileDepthMask) {
                    groupAppendLightOpaque(i);
                }
            }
        }
    }
    GroupMemoryBarrierWithGroupSync();
    const uint numTileSpotLightsOpaque = groupLightCountOpaque - numTilePointLightsOpaque;
    const uint numTileSpotLightsTransparent = groupLightCountTransparent - numTilePointLightsTransparent;


    const uint totalIndexCountOpaque = groupLightCountOpaque + NUM_DECAL_BUCKETS;
    const uint totalIndexCountTransparent = groupLightCountTransparent + NUM_DECAL_BUCKETS;
    const uint totalIndexCount = totalIndexCountOpaque + totalIndexCountTransparent;
    if (csIn.groupIndex == 0)
    {
        InterlockedAdd(tiledCullingIndexCounter[0], totalIndexCount, groupObjectsStartOffset);

        tiledCullingGrid[csIn.groupID.xy] = uint4(
            groupObjectsStartOffset,
            (numTilePointLightsOpaque << 20) | (numTileSpotLightsOpaque << 10),
            groupObjectsStartOffset + totalIndexCountOpaque, // Transparent objects are directly after opaques.
            (numTilePointLightsTransparent << 20) | (numTileSpotLightsTransparent << 10)
        );
    }
    GroupMemoryBarrierWithGroupSync();

    const uint offsetOpaque = groupObjectsStartOffset + 0;
    for (i = csIn.groupIndex; i < totalIndexCountOpaque; i += LIGHT_CULLING_TILE_SIZE * LIGHT_CULLING_TILE_SIZE)
    {
        tiledObjectsIndexList[offsetOpaque + i] = groupObjectsListOpaque[i];        //decal( fix NUM_DECAL_BUCKETS, 8) -> point light (groupLightCountOpaque) -> spot light
    }

    const uint offsetTransparent = groupObjectsStartOffset + totalIndexCountOpaque;
    for (i = csIn.groupIndex; i < totalIndexCountTransparent; i += LIGHT_CULLING_TILE_SIZE * LIGHT_CULLING_TILE_SIZE)
    {
        tiledObjectsIndexList[offsetTransparent + i] = groupObjectsListTransparent[i];
    }
}