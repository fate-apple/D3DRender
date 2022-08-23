#pragma once
#include "Math.h"

struct RandomNumbterGenerator
{
    RandomNumbterGenerator() {}
    RandomNumbterGenerator(uint64 seed) { this->seed = seed; }
    
    uint64 seed;

    // https://en.wikipedia.org/wiki/Xorshift
    inline uint64 randomUint64()
    {
        uint64 x = seed;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        seed = x;
        return x;
    }
};