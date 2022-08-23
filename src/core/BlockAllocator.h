#pragma once
#include <map>
#include "pch.h"
//a lightweight class that handles free memory block management to accommodate variable-size allocation requests.
// Based on https://www.codeproject.com/Articles/1180070/Simple-Variable-Size-Memory-Block-Allocator.
//use map to find the block suitable for request descriptorNum
class BlockAllocator
{
public:
    uint64 availableSize = 0;

    void Initialize(uint64 capacity);
    uint64 Allocate(uint64 requestedSize);
    void Free(uint64 offset, uint64 size);

private:
    struct OffsetValue;
    struct SizeValue;
    using BlockBySizeMap = std::multimap<uint64, SizeValue>;
    using BlockByOffsetMap  = std::map<uint64, OffsetValue>;
    struct OffsetValue
    {
        OffsetValue() = default;
        explicit OffsetValue(const BlockBySizeMap::iterator sizeIterator) : sizeIterator(sizeIterator)  {}

        BlockBySizeMap::iterator sizeIterator;
        [[nodiscard]] uint64 GetSize() const {return sizeIterator->first;}
    };
    struct SizeValue
    {
        SizeValue() = default;
        SizeValue(BlockByOffsetMap::iterator offsetIterator) : offsetIterator(offsetIterator){}
        // size was implicitly stored in key of BlockBySizeMap
        
        BlockByOffsetMap::iterator offsetIterator;

        [[nodiscard]] uint64 GetOffset() const {return offsetIterator->first;}
    };

    BlockByOffsetMap _blockByOffset;
    BlockBySizeMap _blockBySize;

    void AddNewBlock(uint64 offset, uint64 size)
    {
        auto [offsetIt, _] = _blockByOffset.emplace(offset, OffsetValue());
        const auto sizeIt = _blockBySize.emplace(size, offsetIt);
        offsetIt->second.sizeIterator = sizeIt;
    }
    
};
