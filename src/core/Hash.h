#pragma once

#include "math.h"

#include <functional>

// Source: https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
template <typename T>
inline void HashCombine(size_t& seed, const T& v)
{
    seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}


namespace std
{
    template <>
    struct hash<vec2>
    {
        size_t operator()(const vec2& x) const noexcept
        {
            size_t seed = 0;

            HashCombine(seed, x.x);
            HashCombine(seed, x.y);

            return seed;
        }
    };

    template <>
    struct hash<vec3>
    {
        size_t operator()(const vec3& x) const noexcept
        {
            size_t seed = 0;

            HashCombine(seed, x.x);
            HashCombine(seed, x.y);
            HashCombine(seed, x.z);

            return seed;
        }
    };

    template <>
    struct hash<vec4>
    {
        size_t operator()(const vec4& x) const noexcept
        {
            size_t seed = 0;

            HashCombine(seed, x.x);
            HashCombine(seed, x.y);
            HashCombine(seed, x.z);
            HashCombine(seed, x.w);

            return seed;
        }
    };

    template <>
    struct hash<quat>
    {
        size_t operator()(const quat& x) const noexcept
        {
            size_t seed = 0;

            HashCombine(seed, x.x);
            HashCombine(seed, x.y);
            HashCombine(seed, x.z);
            HashCombine(seed, x.w);

            return seed;
        }
    };

    template <>
    struct hash<mat4>
    {
        size_t operator()(const mat4& x) const noexcept
        {
            size_t seed = 0;

            for(uint32 i = 0; i < 16; ++i) {
                HashCombine(seed, x.m[i]);
            }

            return seed;
        }
    };
}