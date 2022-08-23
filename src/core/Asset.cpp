#include "pch.h"
#include "Asset.h"
#include "Random.h"

static RandomNumbterGenerator rng = time(nullptr);
AssetHandle AssetHandle::Generate()
{
    return AssetHandle{rng.randomUint64()};
}