#ifndef CAMERA_H
#define CAMERA_H

struct CameraCB
{
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    mat4 invViewProj;
    mat4 invView;
    mat4 invProj;
    mat4 prevFrameViewProj;
    
    vec4 position;
    vec4 forward;
    vec4 right;
    vec4 up;
    vec4 projectionParams; // nearPlane, farPlane, farPlane / nearPlane, 1 - farPlane / nearPlane
    vec4 viewSpaceTopLeftFrustumVector; // z = -1.
    vec2 extentAtDistanceOne;
    vec2 screenDims;
    vec2 invScreenDims;
    vec2 jitter;
    vec2 prevFrameJitter;



#ifdef HLSL
    // A + B * (1/Z)  = x  =>  Z = B / (x-A).  0 => near, 1 => far. recall we keep m32 = -1
    float DepthBufferDepthToEyeDepth(float depthBufferDepth)
    {
        if (projectionParams.y < 0.f) // Infinite far plane.
            {
            depthBufferDepth = clamp(depthBufferDepth, 0.f, 1.f - 1e-7f); // A depth of 1 is at infinity.
            return -projectionParams.x / (depthBufferDepth - 1.f);
            }
        else
        {
            const float c1 = projectionParams.z;
            const float c0 = projectionParams.w;
            return projectionParams.y / (c0 * depthBufferDepth + c1);
        }
    }

    // NOT normalized. Z-coordinate is 1 unit long.
    float3 RestoreViewDirection(float2 uv)
    {
        float2 offset = extentAtDistanceOne * uv;
        return viewSpaceTopLeftFrustumVector.xyz + float3(offset.x, -offset.y, 0.f);        // top-left at (near) + top-left at (1-near)
    }

    float3 RevertViewSpacePosition(float2 uv, float depth)
    {
        float viewDepth = DepthBufferDepthToEyeDepth(depth);
        return RestoreViewDirection(uv) * viewDepth;
    }

    float3 restoreViewSpacePositionEyeDepth(float2 uv, float eyeDepth)
    {
        return RestoreViewDirection(uv) * eyeDepth;
    }

    float3 RestoreWorldSpacePosition(float2 uv, float depth)
    {
        uv.y = 1.f - uv.y; // Screen uvs start at the top left, so flip y.
        float3 ndc = float3(uv * 2.f - float2(1.f, 1.f), depth);
        float4 homPosition = mul(invViewProj, float4(ndc, 1.f));
        float3 position = homPosition.xyz / homPosition.w;
        return position;
    }
#endif
};

#endif
