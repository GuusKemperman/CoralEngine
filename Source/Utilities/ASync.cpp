#include "Precomp.h"
#include "Utilities/ASync.h"

#include <forward_list>
#include <mutex>
#include <thread>

namespace
{
	struct Worker;
}

namespace CE::Internal
{
	struct Job
	{
		Job(std::function<void()>&& workload, std::weak_ptr<Worker>&& worker);

		std::function<void()> mWorkload{};
		std::weak_ptr<Worker> mWorker{};
		bool mIsCancelled{};
		std::mutex mMutex{};
	};
}

namespace
{
	struct Worker
	{
		void RunWorkerThread();

		std::thread mThread{
			[this]
			{
				RunWorkerThread();
			}
		};
		std::list<std::shared_ptr<CE::Internal::Job>> mJobs{};
		std::mutex mJobsMutex{};
	};

	void DoJob(const std::shared_ptr<CE::Internal::Job> job);
	void AddJob(std::shared_ptr<Worker>&& worker, std::function<void()>&& workload);

	std::vector<std::shared_ptr<Worker>> sWorkers{};
	std::mutex sWorkersMutex{};
}

CE::Internal::Job::Job(std::function<void()>&& workload, std::weak_ptr<Worker>&& worker) :
	mWorkload(std::move(workload)),
	mWorker(std::move(worker))
{
}

void Worker::RunWorkerThread()
{
	while (true)
	{
		if (mJobs.empty())
		{
			continue;
		}

		std::shared_ptr<CE::Internal::Job> jobToDo{};

		mJobsMutex.lock();

		if (mJobs.empty())
		{
			mJobsMutex.unlock();
			continue;
		}

		jobToDo = mJobs.front();
		mJobsMutex.unlock();

		DoJob(std::move(jobToDo));
	}
}

CE::ASyncThread::ASyncThread(std::function<void()>&& work)
{
	std::shared_ptr<Worker> bestWorker{};

	sWorkersMutex.lock();

	// Find the worker with the least amount of jobs.
	for (const std::shared_ptr<Worker>& worker : sWorkers)
	{
		// If we find a thread with no work left, we assign the job.
		if (worker->mJobs.empty())
		{
			bestWorker = std::move(worker);
			break;
		}

		if (bestWorker == nullptr
			|| worker->mJobs.size() < bestWorker->mJobs.size())
		{
			bestWorker = std::move(worker);
		}
	}

	if (bestWorker == nullptr
		|| bestWorker->mJobs.size() > 4)
	{
		bestWorker = sWorkers.emplace_back();
	}

	sWorkersMutex.unlock();

	AddJob(std::move(bestWorker), std::move(work));
}

CE::ASyncThread::~ASyncThread()
{
	ASSERT(!WasLaunched());
}

bool CE::ASyncThread::WasLaunched() const
{
	return mJob != nullptr;
}

void CE::ASyncThread::Cancel()
{
	ASSERT(WasLaunched());
	mJob->mIsCancelled = true;
	Join();
}

void CE::ASyncThread::Join()
{
	ASSERT(WasLaunched());
	DoJob(mJob);
	mJob = nullptr;
}

namespace
{
	void DoJob(const std::shared_ptr<CE::Internal::Job> job)
	{
		if (job == nullptr
			|| job->mIsCancelled)
		{
			return;
		}

		const std::shared_ptr thread{ job->mWorker };

		thread->mJobsMutex.lock();
		thread->mJobs.remove(job);
		thread->mJobsMutex.unlock();

		job->mMutex.lock();

		// Could've been cancelled from another thread
		// during the last few lines
		if (!job->mIsCancelled)
		{
			job->mWorkload();
		}

		job->mMutex.unlock();
	}

	void AddJob(std::shared_ptr<Worker>&& worker, std::function<void()>&& workload)
	{
		worker->mJobsMutex.lock();
		worker->mJobs.emplace_back(std::make_shared<CE::Internal::Job>(std::move(workload), worker));
		worker->mJobsMutex.unlock();
	}
}