#ifndef LIGHT_CULLING_H
#define LIGHT_CULLING_H

#define LIGHT_CULLING_TILE_SIZE 16

#define MAX_NUM_LIGHTS_PER_TILE 256
#define MAX_NUM_TOTAL_DECALS 256        //per frame

#define NUM_DECAL_BUCKETS (MAX_NUM_TOTAL_DECALS / 32)
#define TILE_LIGHT_OFFSET (NUM_DECAL_BUCKETS)       //order by : decals => lights
#define MAX_NUM_INDICES_PER_TILE (MAX_NUM_LIGHTS_PER_TILE + NUM_DECAL_BUCKETS)

struct LightCullingViewFrustumPlane
{
    vec3 N;
    float d;        //distance from origin to plane
};
struct LightCullingViewFrustum
{
    LightCullingViewFrustumPlane planes[4]; // Left, right, top, bottom frustum planes.
};

struct FrustaCB
{
    uint32 numThreadsX;
    uint32 numThreadsY;
};

struct LightCullingCB
{
    uint32 numThreadGroupsX;
    uint32 numPointLights;
    uint32 numSpotLights;
    uint32 numDecals;
};

#define LIGHT_CULLING_RS \
"RootFlags(0), " \
"CBV(b0), " \
"RootConstants(b1, num32BitConstants = 4), " \
"DescriptorTable( SRV(t0, numDescriptors = 5), UAV(u0, numDescriptors = 3) )"

#define WORLD_SPACE_TILED_FRUSTA_RS \
"RootFlags(0), " \
"CBV(b0), " \
"RootConstants(b1, num32BitConstants = 2), " \
"UAV(u0)"

#define WORLD_SPACE_TILED_FRUSTA_RS_CAMERA      0
#define WORLD_SPACE_TILED_FRUSTA_RS_CB          1
#define WORLD_SPACE_TILED_FRUSTA_RS_FRUSTA_UAV  2

#define LIGHT_CULLING_RS_CAMERA                 0
#define LIGHT_CULLING_RS_CB                     1
#define LIGHT_CULLING_RS_SRV_UAV                2

#endif