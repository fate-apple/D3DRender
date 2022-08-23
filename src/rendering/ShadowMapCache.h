#pragma once


struct ShadowMapViewport
{
    uint16 x,y;
    uint16 size;
};

std::pair<ShadowMapViewport, bool> AssignShadowMapViewport(uint64 lightID, uint64 lightMovementHash, uint32 size);