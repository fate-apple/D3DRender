#include "pch.h"
#include "RaytracingTLAS.h"

void RaytracingTLAS::Initialize(RaytracingAsMode inRebuildMode)
{
    this->rebuildMode = inRebuildMode;
    //TODO
    allInstances.reserve(4096);
}
