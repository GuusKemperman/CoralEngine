#pragma once
#include <mutex>
#include <queue>
#include <thread>

namespace CE
{
	/**
	 * \brief Runs jobs fully asynchronously from the engine. Does not keep threads
	 * running to spare your PC and keep the rest of the engine running smooth.
	 */
	class JobManager :
		public EngineSubsystem<JobManager>
	{
		friend EngineSubsystem<JobManager>;
		~JobManager();

	public:
		/**
		 * \brief Adds a job to be executed asynchronously from the rest of the engine.
		 *
		 * Note that you cannot call AddWork from multiple threads.
		 *
		 * \param job The function to be executed on another thread
		 */
		void AddWork(std::function<void()>&& job);

		/**
		 * \brief Will halt execution of the main thread until all jobs are completed.
		 */
		void FinishAllJobs();

	private:
		void DoWork(bool& isFinished);

		static constexpr int sMaxThreads = 8;
		static constexpr int sMinNumOfJobsPerThread = 1;

		std::queue<std::function<void()>> mJobs{};
		std::mutex mJobMutex{};

		struct Thread
		{
			std::thread mThread{};
			bool mIsFinished{};
		};

		std::array<Thread, sMaxThreads> mThreads{};
	};

	inline JobManager::~JobManager()
	{
		FinishAllJobs();
	}

	inline void JobManager::AddWork(std::function<void()>&& job)
	{
		int numOfJobs{};
		{
			std::lock_guard _{ mJobMutex };
			mJobs.emplace(std::move(job));
			numOfJobs = static_cast<int>(mJobs.size());
		}

		int numOfThreadsRunning{};

		// Let's see if we need to spin up some more threads
		for (Thread& thread : mThreads)
		{
			if (thread.mIsFinished
				&& thread.mThread.joinable())
			{
				thread.mThread.join();
				thread.mIsFinished = false;
			}
			numOfThreadsRunning += thread.mThread.joinable();
		}

		const int numOfAvailThreads = sMaxThreads - numOfThreadsRunning;
		const int numOfThreadsToSpinUp = std::min(numOfAvailThreads, (numOfJobs - (numOfThreadsRunning * sMinNumOfJobsPerThread)));

		for (int numOfThreadsStarted = 0, threadIndex = 0; numOfThreadsStarted < numOfThreadsToSpinUp && threadIndex < sMaxThreads; numOfThreadsStarted++, threadIndex++)
		{
			Thread& thread = mThreads[threadIndex];

			if (!thread.mThread.joinable())
			{
				thread.mThread = std::thread{ [this, &thread] { DoWork(thread.mIsFinished); } };
			}
		}
	}

	inline void JobManager::FinishAllJobs()
	{
		for (Thread& thread : mThreads)
		{
			if (thread.mThread.joinable())
			{
				thread.mThread.join();
			}
		}
	}

	inline void JobManager::DoWork(bool& isFinished)
	{
		mJobMutex.lock();

		if (mJobs.empty())
		{
			mJobMutex.unlock();
			return;
		}

		{
			std::function<void()> job = std::move(mJobs.front());
			mJobs.pop();
			mJobMutex.unlock();

			job();
		}

		DoWork(isFinished);
		isFinished = true;
	}
}
