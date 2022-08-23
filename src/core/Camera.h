#pragma once
#include "math.h"

enum CameraType
{
    cameraTypeIngame,
    cameraTypeCalibrated,
};

struct RenderCamera
{
    quat rotation;
    vec3 position;

    float nearPlane;
    float farPlane = -1.f;      //negative stands for infinity. So we keep near & far positive otherwise. As camera look at -z axis, -near & - far is the frusta plane
    uint32 width, height;
    float aspect;

    CameraType type;

    float verticalFOV;      //angle in rad
    float fx, fy, cx, cy;
    
    mat4 view;
    mat4 proj;
    mat4 viewProj;

    mat4 invView;
    mat4 invProj;
    mat4 invViewProj;

    void InitializeIngame(vec3 inPosition, quat inRotation, float inVerticalFOV, float inNearPlane, float inFarPlane = -1.f);
    void InitializeCalibrated(vec3 position, quat rotation, uint32 width, uint32 height, float fx, float fy, float cx, float cy, float nearPlane, float farPlace = -1.f);
    void SetViewport(uint32 width, uint32 height);
    void UpdateMatrices();
};