#ifndef DEPTH_ONLY_RS_HLSLI
#define DEPTH_ONLY_RS_HLSLI

struct DepthOnlyObjectIdCB
{
    uint32 id;
};

struct DepthOnlyCameraJitterCB
{
    vec2 jitter;
    vec2 preFrameJitter;
};

struct DepthOnlyTransformCB
{
    mat4 mvp;       //we use col major. (p*v*m) * (objectPos) actually
    mat4 prevFrameMVP;
};


#define DEPTH_ONLY_RS \
"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |" \
"DENY_HULL_SHADER_ROOT_ACCESS |" \
"DENY_DOMAIN_SHADER_ROOT_ACCESS |" \
"DENY_GEOMETRY_SHADER_ROOT_ACCESS)," \
"RootConstants(num32BitConstants = 32, b0, visibility = SHADER_VISIBILITY_VERTEX), " \
"RootConstants(num32BitConstants = 1, b1, visibility = SHADER_VISIBILITY_PIXEL), " \
"RootConstants(num32BitConstants = 4, b2, visibility = SHADER_VISIBILITY_PIXEL)"

#define ANIMATED_DEPTH_ONLY_RS \
"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |" \
"DENY_HULL_SHADER_ROOT_ACCESS |" \
"DENY_DOMAIN_SHADER_ROOT_ACCESS |" \
"DENY_GEOMETRY_SHADER_ROOT_ACCESS)," \
"RootConstants(num32BitConstants = 32, b0, visibility = SHADER_VISIBILITY_VERTEX), " \
"RootConstants(num32BitConstants = 1, b1, visibility = SHADER_VISIBILITY_PIXEL), " \
"RootConstants(num32BitConstants = 4, b2, visibility = SHADER_VISIBILITY_PIXEL), " \
"SRV(t0)"

#define DEPTH_ONLY_RS_MVP       0
#define DEPTH_ONLY_RS_OBJECT_ID 1
#define DEPTH_ONLY_RS_CAMERA_JITTER 2
#define DEPTH_ONLY_RS_PREV_FRAME_POSITIONS 3

#define SHADOW_RS_MVP           0
#endif