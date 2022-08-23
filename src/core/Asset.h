#pragma once
#include "pch.h"
#include "random.h"

struct AssetHandle
{
    AssetHandle() : id(0){}
    AssetHandle(uint64 id) : id(id){}
    static  AssetHandle Generate();
    
    uint64 id;
    
    operator bool() const { return id != 0;}
};

inline bool operator==(const AssetHandle& a, const AssetHandle& b)
{
    return a.id == b.id;
}

template<>
struct std::hash<AssetHandle>
{
    size_t operator()(const AssetHandle& x) const noexcept
    {
        return std::hash<uint64>()(x.id);
    }
};

