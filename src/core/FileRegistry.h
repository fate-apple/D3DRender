#pragma once
#include "Asset.h"

void InitializeFileRegistry();
AssetHandle GetAssetHandleFromPath(const fs::path& path);
fs::path GetPathFromAssetHandle(AssetHandle handle);