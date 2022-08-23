#pragma once
#include "pch.h"
#define KB(n) (1024ull * (n))
#define MB(n) (1024ull * KB(n))
#define GB(n) (1024ull * MB(n))


static uint64 AlignTo(uint64 currentOffset, uint64 alignment)
{
    uint64 mask = alignment - 1;
    uint64 misalignment = currentOffset & mask;
    uint64 adjustment = (misalignment == 0) ? 0 : (alignment - misalignment);
    return currentOffset + adjustment;
}

struct MemoryMarker
{
    uint64 before;
};

/**
 * \brief use system API to get a big chunk of memory(512 *  sizeof(DxPage)) when Initialize.
 * Control Allocate Request of  heap memory in application
 */
class MemoryArena
{
public:
    MemoryArena() = default;
    MemoryArena(const MemoryArena&) = delete;
    MemoryArena& operator=(MemoryArena other) = delete;
    MemoryArena(MemoryArena&&) = default;
    MemoryArena& operator=(MemoryArena&& other) = default;
    ~MemoryArena() { Reset(true); }

    void Initialize(uint64 minimumBlockSize = 0, uint64 inReserveSize = GB(8));
    void ResetToMarker(MemoryMarker marker);
    void Reset(bool freeMemory = false);

    void EnsureFreeSize(uint64 size);
    void* Allocate(uint64 size, uint64 alignment = 1, bool clearToZero = false);
    template <typename T>
    T* Allocate(uint32 count = 1, bool clearToZero = false)
    {
        return static_cast<T*>( Allocate(sizeof(T) * count, alignof(T), clearToZero) );
    }
    void * GetCurrent(uint64 alignment =1);
    template< typename  T>
    T* GetCurrent()
    {
        return static_cast<T*>(GetCurrent(alignof(T)));
    }
    void SetCurrentTo(void* ptr);

protected:
    uint8* memory = nullptr; //start. byte*
    uint64 committedMemory = 0;

    // current offset
    uint64 current = 0;
    uint64 sizeLeftCurrent = 0; // 0 for initial
    uint64 sizeLeftTotal = 0; // reserveSize for initial

    uint64 pageSize = 0;
    uint64 minimumBlockSize = 0;

    uint64 reserveSize = 0;
};