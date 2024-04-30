#pragma once

namespace CE
{
	namespace Internal
	{
		struct Job;
	}

	// Like a regular thread, except this one
	// does not slow down the main thread.
	class ASyncThread
	{
	public:
		ASyncThread() = default;
		ASyncThread(std::function<void()>&& work);

		ASyncThread(const ASyncThread&) = delete;
		ASyncThread(ASyncThread&&) noexcept = default;

		ASyncThread& operator=(const ASyncThread&) = delete;
		ASyncThread& operator=(ASyncThread&&) noexcept = default;

		~ASyncThread();

		bool WasLaunched() const;

		void CancelOrJoin();
		void CancelOrDetach();
		void Join();
		void Detach();

	private:
		std::shared_ptr<Internal::Job> mJob{};
	};

	template<typename T>
	class ASyncFuture
	{
	public:
		ASyncFuture() = default;
		ASyncFuture(std::function<T()>&& work) :
			mValue(std::make_shared<std::optional<T>>()),
			mThread(
				[value = mValue, work]
				{
					const_cast<std::shared_ptr<std::optional<T>>&>(value)->emplace(work());
				}
			)
		{
		}

		ASyncFuture(const ASyncFuture&) = delete;
		ASyncFuture(ASyncFuture&&) noexcept = default;

		ASyncFuture& operator=(const ASyncFuture&) = delete;
		ASyncFuture& operator=(ASyncFuture&&) noexcept = default;

		ASyncThread& GetThread() { return mThread; }
		const ASyncThread& GetThread() const { return mThread; }

		bool IsReady() const { return mValue != nullptr && mValue->has_value(); }

		T& Get() { return **mValue; }

	private:
		std::shared_ptr<std::optional<T>> mValue{};
		ASyncThread mThread{};
	};
}
