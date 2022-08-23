#pragma once
#include "ProfilingInternal.h"

#define CPU_PROFILE_BLOCK(name) ProfileCpuBlockRecorder __PROFILE_BLOCK__COUNTER__(name)

struct ProfileCpuBlockRecorder
{
    const char* name;
    ProfileCpuBlockRecorder(const char* name)
        : name(name)
    {
        //recordProfileEvent(profile_event_begin_block, name);
    }
};