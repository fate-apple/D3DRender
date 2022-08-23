#pragma once
#include <functional>

static uint32 AtomicIncrement(volatile uint32& a) {	return InterlockedIncrement((volatile LONG*)&a) - 1; }
static uint64 AtomicIncrement(volatile uint64& a) {	return InterlockedIncrement64((volatile LONG64*)&a) - 1; }
static uint32 AtomicDecrement(volatile uint32& a) {	return InterlockedDecrement((volatile LONG*)&a) + 1; }
static uint64 AtomicDecrement(volatile uint64& a) {	return InterlockedDecrement64((volatile LONG64*)&a) + 1; }
static uint32 AtomicAdd(volatile uint32& a, uint32 b){return InterlockedAdd(reinterpret_cast<volatile LONG*>(&a), (LONG)b)-b;}
static uint64 AtomicAdd(volatile uint64& a, uint64 b){return InterlockedAdd64((volatile LONG64*)(&a), (LONG64)b)-b;}

struct ThreadJobContext
{
    volatile uint32 numJobs = 0;

    void AddWork(const std::function<void()>& callBackFunc);
    void WaitForWorkCompletion();
};

void InitializeJobSystem();