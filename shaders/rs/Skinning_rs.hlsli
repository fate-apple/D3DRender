#ifndef SKINNING_RS_HLSLI
#define SKINNING_RS_HLSLI


struct SkinningCB
{
    
    uint32 firstJoint;
    uint32 numJoints;
    uint32 firstVertex;
    uint32 numVertices;
    uint32 writeOffset;     //output start index;
};

struct MeshPosition
{
    vec3 position;
};

struct MeshOtherInfo
{
    vec3 normal;
    vec3 tangent;
    vec2 uv;
};

struct SkinnedMeshPosition
{
    vec3 position;
};

struct SkinnedMeshOtherInfo
{
    vec3 normal;
    uint32 skinIndices;     //4 elements, each 8 bit
    vec3 tangent;
    uint32 skinWeights;
    vec2 uv;
};


#define SKINNING_RS_CB			                0
#define SKINNING_RS_INPUT_VERTEX_BUFFER0        1
#define SKINNING_RS_INPUT_VERTEX_BUFFER1        2
#define SKINNING_RS_MATRICES                    3
#define SKINNING_RS_OUTPUT0                     4
#define SKINNING_RS_OUTPUT1                     5


#define SKINNING_RS \
    "RootConstants(b0, num32bitConstants = 5), "    \
    "SRV(t0), "     \
    "SRV(t1), "     \
    "SRV(t2), "     \
    "UAV(u0), "     \
    "UAV(u1) "     

#endif