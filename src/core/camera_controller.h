#pragma once

#include "camera.h"
#include "input.h"
#include "physics/BoundingVolume.h"


struct camera_controller
{
	void initialize(RenderCamera* camera) { this->camera = camera; }

	// Returns true, if camera is moved, and therefore input is captured.
	bool centerCameraOnObject(const BoundingBox& aabb);
	bool update(const UserInput& input, uint32 viewportWidth, uint32 viewportHeight, float dt);

	RenderCamera* camera;

private:
	float orbitRadius = 10.f;


	float centeringTime = -1.f;

	vec3 centeringPositionStart;
	vec3 centeringPositionTarget;
};

