#pragma once

#include "DxContext.h"
#include "pch.h"
#include "core/ProfilingCPU.h"
#include "core/Thread.h"


#if ENABLE_DX_PROFILING


#define DX_PROFILE_BLOCK(cl, name) DxProfileBlock __DX_PROFILE_BLOCK__COUNTER__(cl, name)

#define MAX_NUM_DX_PROFILE_BLOCKS 2048
#define MAX_NUM_DX_PROFILE_EVENTS (MAX_NUM_DX_PROFILE_BLOCKS * 2) // One for start and end.
extern ProfileEvent dxProfileEvents[NUM_BUFFERED_FRAMES][MAX_NUM_DX_PROFILE_EVENTS];
struct DxProfileBlock
{
    DxCommandList* cl;
    const char* name;
    DxProfileBlock(DxCommandList* cl, const char* name)
        : cl(cl), name(name)
    {
        uint32 queryIndex = AtomicIncrement(dxContext.timestampQueryIndex[dxContext.bufferedFrameID]);
    }
};
#else

#define DX_PROFILE_BLOCK(...)

#endif

#define PROFILE_ALL(cl, name) DX_PROFILE_BLOCK(cl, name); CPU_PROFILE_BLOCK(name);