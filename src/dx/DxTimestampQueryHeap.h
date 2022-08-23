#pragma once
#include "Dx.h"

struct DxTimestampQueryHeap
{
    // A query heap holds an array of queries, referenced by indexes
    Dx12QueryHeap heap;

    void Initialize(uint32 maxCount);
};
