#include "pch.h"
#include "Camera.h"

void RenderCamera::InitializeIngame(vec3 inPosition, quat inRotation, float inVerticalFOV, float inNearPlane, float inFarPlane)
{
    type = cameraTypeIngame;
    this->position = inPosition;
    this->rotation = inRotation;
    this->verticalFOV = inVerticalFOV;
    this->nearPlane = inNearPlane;
    this->farPlane = inFarPlane;
}

void RenderCamera::InitializeCalibrated(vec3 position, quat rotation, uint32 width, uint32 height, float fx, float fy, float cx, float cy, float nearPlane, float farPlane)
{
    type = cameraTypeCalibrated;
    this->position = position;
    this->rotation = rotation;
    this->fx = fx;
    this->fy = fy;
    this->cx = cx;
    this->cy = cy;
    this->width = width;
    this->height = height;
    this->nearPlane = nearPlane;
    this->farPlane = farPlane;
    this->aspect = static_cast<float>(width) / static_cast<float>(height);
}

void RenderCamera::SetViewport(uint32 width, uint32 height)
{
    this->width = width;
    this->height = height;
    aspect = static_cast<float>(width) / static_cast<float>(height);
}

void RenderCamera::UpdateMatrices()
{
    //view matrix look at -z axis; we use near and far positive, and a negative far plane stands for infinity 
    //so z = -near => z' = 0, z = -far => z' = 1;
    if(type == cameraTypeIngame) {
        proj = CreatePerspectiveProjectionMatrix(verticalFOV, aspect, nearPlane, farPlane);
    }
    else if (type == cameraTypeCalibrated) {
        // Calculate matrix by focal lengths and camera-central point
        proj = CreatePerspectiveProjectionMatrix((float)width, (float)height, fx, fy, cx, cy, nearPlane, farPlane);
    }
    else {
        std::cerr << "UpdateMatrices: unknown camera type";
    }
    invProj = InvertPerspectiveProjectionMatrix(proj);
    // look at -z axis
    view = CreateViewMatrix(position, rotation);
    invView = InvertAffine(view);
    viewProj = proj * view;
    invViewProj = invView * invProj;
}
