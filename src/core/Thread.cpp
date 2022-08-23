#include "pch.h"
#include "Thread.h"

#include <thread>

struct WorkQueueEntry
{
    std::function<void()> callback;
    ThreadJobContext* context;
};

/**
 * \brief  Ring queue, key is never let nextItemToRead run after nextItemToWrite and block to prevent their equal.
 * Push impel nextItemToWrite and Perform work impel nextItemToRead;
 * \tparam T                data type
 * \tparam capacity         max dataElement count
 */
template <typename T, uint32 capacity>
struct ThreadSafeRingBuffer
{
    uint32 nextItemToRead = 0;
    uint32 nextItemToWrite = 0;
    T data[capacity];
    std::mutex mutex;

    bool push_back(const T& t)
    {
        bool result = false;
        mutex.lock();
        uint32 next = (nextItemToWrite + 1) % capacity;
        if(next != nextItemToRead) {
            data[nextItemToWrite] = t;
            nextItemToWrite = next;
            result = true;
        }
        mutex.unlock();
        return result;
    }

    bool pop_front(T& t)
    {
        bool result = false;
        mutex.lock();
        if(nextItemToRead != nextItemToWrite) {
            t = data[nextItemToRead];
            nextItemToRead = (nextItemToRead + 1) % capacity;
            result = true;
        }
        mutex.unlock();
        return result;
    }
};

static HANDLE semaphoreHandle;
static ThreadSafeRingBuffer<WorkQueueEntry, 256> queue;

/**
 * \brief  Get an entry and start its work;
 * \return  Is get entry succeed.
 */
static bool PerformWork()
{
    WorkQueueEntry entry;
    if(queue.pop_front(entry)) {
        entry.callback();
        AtomicDecrement(entry.context->numJobs);

        return true;
    }

    return false;
}

static void WorkerThreadProc()
{
    while(true) {
        if(!PerformWork()) {
            WaitForSingleObjectEx(semaphoreHandle, INFINITE, FALSE);
        }
    }
}

void InitializeJobSystem()
{
    HANDLE handle = GetCurrentThread();
    SetThreadAffinityMask(handle, 1);
    //An affinity mask is a bit mask indicating what processor(s) a thread or process should be run on by the scheduler of an operating system.
    SetThreadPriority(handle, THREAD_PRIORITY_HIGHEST);
    CloseHandle(handle);

    uint32 numThreads = std::thread::hardware_concurrency();
    numThreads = std::clamp(numThreads, 1u, 8u);
    //A semaphore object is a synchronization object that maintains a count between zero and a specified maximum value.
    //The count is decremented each time a thread completes a wait for the semaphore object and incremented each time a thread releases the semaphore
    semaphoreHandle = CreateSemaphoreEx(0, 0, numThreads, nullptr, 0, SEMAPHORE_ALL_ACCESS);
    //Creates or opens a named or unnamed semaphore object and returns a handle to the object.
    for(uint32 i(0); i < numThreads; ++i) {
        std::thread thread(WorkerThreadProc);
        HANDLE handle = thread.native_handle();
        // 1 is main Thread
        uint64 affinityMask = 1ull << (i + 1);
        SetThreadAffinityMask(handle, affinityMask);
        SetThreadDescription(handle, L"Worker Thread");
        thread.detach();
    }
}

//Create entry and push_back; if fail, perform work until circle list has free pos;
void ThreadJobContext::AddWork(const std::function<void()>& callBackFunc)
{
    WorkQueueEntry entry;
    entry.callback = callBackFunc;
    entry.context = this;
    AtomicIncrement(numJobs);
    while(!queue.push_back(entry)) {
        PerformWork();
    }
    ReleaseSemaphore(semaphoreHandle, 1, nullptr);
}

// uninterrupted call PerformWork() until all jobs belong to this context get executed;
void ThreadJobContext::WaitForWorkCompletion()
{
    while(numJobs) {
        if(!PerformWork()) {
            while(numJobs) {}
            break;
        }
    }
}