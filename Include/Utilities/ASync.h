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

		~ASyncThread();

		bool WasLaunched() const;

		void CancelOrJoin();
		void CancelOrDetach();
		void Join();
		void Detach();

	private:
		std::shared_ptr<Internal::Job> mJob{};
	};
}
