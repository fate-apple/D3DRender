#pragma once

#define COMPOSITE_VARNAME(a, b) a##b

enum ProfileEventType : uint16
{
    None,
    frameMarker,
    beginBlock,
    endBlock
};

struct ProfileEvent
{
    const char* name;
    uint64 timeStamp;
    uint32 threadID;
    ProfileEventType type;
    uint16 clType;      //TODO: sjw. gpu profiler
};