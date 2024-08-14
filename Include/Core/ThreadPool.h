#pragma once
#include <future>
#include <queue>

namespace CE
{
	class ThreadPool final :
		public EngineSubsystem<ThreadPool>
	{
		friend EngineSubsystem;

		ThreadPool();
		~ThreadPool(); // joins all threads
	public:

		template<class F, class... A>
		decltype(auto) Enqueue(F&& callable, A &&...arguments);

		size_t NumberOfThreads() const { return mThreads.size(); }

	private:
		std::vector<std::thread> mThreads{};
		std::queue<std::packaged_task<void()>> mTasks{};
		std::mutex mMutex{};
		std::condition_variable mCondition{};
		bool mStopped{};
	};

	// add new work item to the pool
	template<class F, class... Args>
	decltype(auto) ThreadPool::Enqueue(F&& f, Args&&... args)
	{
		using return_type = std::invoke_result_t<F, Args...>;

		auto task = std::make_shared<std::packaged_task<return_type()>>(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);

		std::future<return_type> res = task->get_future();
		{
			std::unique_lock lock{ mMutex };
			mTasks.emplace([task]{ (*task)(); });
		}
		mCondition.notify_one();
		return res;
	}


	template<typename Future>
	bool IsFutureReady(const Future& f)
	{
		return f.valid() && f.wait_for(std::chrono::seconds{}) == std::future_status::ready;
	}
}