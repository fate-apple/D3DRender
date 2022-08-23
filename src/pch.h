// ReSharper disable CppInconsistentNaming
#pragma once
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <assert.h>
#include <Windows.h>
#include <iostream>
#include <limits>
#include <wrl.h>
#include <filesystem>
#include <mutex>

namespace fs = std::filesystem;
using uint8 = unsigned char;
using uint16 = unsigned short;
// using uint32 = unsigned int;
// get support with ide in hlsli. confused though
typedef unsigned int uint32;
using uint64 = unsigned long long;
using int64 = long long;
using int8 = char;
using int32 = int;
using int16 = short;
using wchar = wchar_t;

template <typename T>
using com = Microsoft::WRL::ComPtr<T>;
template <typename T> using ref = std::shared_ptr<T>;
template <typename T> using weakRef = std::weak_ptr<T>;
template<typename  T, class... Args>
inline ref<T> makeRef(Args&&... args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

#define arraySize(arr) (sizeof(arr) / sizeof((arr)[0]))
//#define arraySize(arr) std::size(arr)       //Only dynamic
//TODO: sjw. SetBit macro conflict with external file like imgui. use namespace?
#define setBit(mask, bit) mask |= (1 << (bit))
#define unSetBit(mask, bit) mask ^= (1 << (bit))

template <typename T>
constexpr inline auto min(T a, T b)
{
    return (a < b) ? a : b;
}

template <typename T>
constexpr inline auto max(T a, T b)
{
    return (a < b) ? b : a;
}
static void CheckResult(HRESULT hr)
{
    assert(SUCCEEDED(hr));
}

