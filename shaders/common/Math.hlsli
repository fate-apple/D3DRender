#ifndef MATH_H
#define MATH_H


#define M_PI 3.1415926f

#define M_INV_PI 0.3183099f
#define M_INV_PI2 0.159154943

static float square(float v) { return v * v; }
static float inverseLerp(float l, float u, float v) { return (v - l) / (u - l); }
static float remap(float v, float oldL, float oldU, float newL, float newU) { return lerp(newL, newU, inverseLerp(oldL, oldU, v)); }

static uint flatten2D(uint2 coord, uint width)
{
    return coord.x + coord.y * width;
}

// Flattened array index to 2D array index.
static uint2 unflatten2D(uint idx, uint2 dim)
{
    return uint2(idx % dim.x, idx / dim.x);
}

static uint2 unflatten2D(uint idx, uint width)
{
    return uint2(idx % width, idx / width);
}

inline bool isSaturated(float a) { return a == saturate(a); }
inline bool isSaturated(float2 a) { return isSaturated(a.x) && isSaturated(a.y); }
inline bool isSaturated(float3 a) { return isSaturated(a.x) && isSaturated(a.y) && isSaturated(a.z); }
inline bool isSaturated(float4 a) { return isSaturated(a.x) && isSaturated(a.y) && isSaturated(a.z) && isSaturated(a.w); }
#endif
