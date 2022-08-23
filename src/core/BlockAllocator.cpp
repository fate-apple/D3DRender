#include "pch.h"
#include "BlockAllocator.h"

void BlockAllocator::Initialize(uint64 capacity)
{
    availableSize = capacity;
    AddNewBlock(0, capacity);
}

uint64 BlockAllocator::Allocate(uint64 requestedSize)
{
    const auto minBlockIt = _blockBySize.lower_bound(requestedSize);      // whose key is not considered to go before k
    if(minBlockIt == _blockBySize.end()) {
        return UINT64_MAX;
    }

    const auto sizeValue = minBlockIt->second;
    const auto offset = sizeValue.GetOffset();        //OffsetIt key
    const auto size = minBlockIt-> first;

    const auto newOffset = offset + requestedSize;
    const auto newSize = size - requestedSize;
    _blockBySize.erase(minBlockIt);
    _blockByOffset.erase(sizeValue.offsetIterator);
    if(newSize > 0) {
        AddNewBlock(newOffset, newSize);
    }
    availableSize -= requestedSize;
    return offset;
}


void BlockAllocator::Free(uint64 offset, uint64 size)
{
    ;
}
