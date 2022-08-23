#pragma once

#include "core/asset.h"
#include "physics/BoundingVolume.h"
#include "dx/DxBuffer.h"
#include "animation/Animation.h"
#include "mesh_builder.h"

struct PbrMaterial;


struct submesh
{
	SubmeshInfo info;
	BoundingBox aabb; // In composite's local space.
	trs transform;

	ref<PbrMaterial> material;
	std::string name;
};

struct composite_mesh
{
	std::vector<submesh> submeshes;
	AnimationSkelon skeleton;
	DxMesh mesh;
	BoundingBox aabb;

	AssetHandle handle;
	uint32 flags;
};


ref<composite_mesh> loadMeshFromFile(const fs::path& sceneFilename, uint32 flags = mesh_creation_flags_default);
ref<composite_mesh> loadMeshFromHandle(AssetHandle handle, uint32 flags = mesh_creation_flags_default);

// Same function but with different default flags (includes skin).
inline ref<composite_mesh> loadAnimatedMeshFromFile(const fs::path& sceneFilename, uint32 flags = mesh_creation_flags_animated)
{
	return loadMeshFromFile(sceneFilename, flags);
}

struct raster_component
{
	ref<composite_mesh> mesh;
};
