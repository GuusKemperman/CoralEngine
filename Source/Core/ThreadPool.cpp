#include "Precomp.h"
#include "Core/ThreadPool.h"

CE::ThreadPool::ThreadPool()
{
    const uint32 numOfHardwareThreads = std::max(4u, std::thread::hardware_concurrency());
    LOG(LogCore, Verbose, "Threadpool has {} thread(s)", numOfHardwareThreads);

    mThreads.reserve(numOfHardwareThreads);
    for (std::size_t i = 0; i < numOfHardwareThreads; ++i)
    {
        mThreads.emplace_back(
            [this]
            {
                while (true)
                {
                    std::packaged_task<void()> task;
                    {
                        std::unique_lock uniqueLock(mMutex);
                        mCondition.wait(uniqueLock, [this] { return mTasks.empty() == false || mStopped == true; });
                        if (mTasks.empty() == false)
                        {
                            task = std::move(mTasks.front());
                            // attention! tasks_.front() moved
                            mTasks.pop();
                        }
                        else
                        {  // stopped_ == true (necessarily)
                            return;
                        }
                    }
                    task();
                }
            });
    }
}

CE::ThreadPool::~ThreadPool()
{
    {
        std::lock_guard lockGuard(mMutex);
        mStopped = true;
    }
    mCondition.notify_all();
    for (std::thread& worker : mThreads)
    {
        worker.join();
    }
}
