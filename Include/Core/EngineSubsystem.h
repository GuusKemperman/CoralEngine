#pragma once

namespace CE
{
	template <class Derived>
	class EngineSubsystem
	{
	protected:
		friend class Engine;
		EngineSubsystem() = default;  // If you declare a constructor, ALWAYS make it private!
		virtual ~EngineSubsystem() = default;
		EngineSubsystem(const EngineSubsystem&) = delete;
		EngineSubsystem& operator=(const EngineSubsystem&) = delete;

		virtual void PostConstruct() {};

		static inline Derived* sInstance = nullptr;

		template <typename... Args>
		static Derived& StartUp(Args&&... args)
		{
			assert(sInstance == nullptr);
			sInstance = new Derived(std::forward<Args>(args)...);
			sInstance->PostConstruct();
			return *sInstance;
		}

		static void ShutDown()
		{
			delete sInstance;
		}

	public:
		[[nodiscard]] static Derived& Get()
		{
			assert(sInstance != nullptr && "Instance was nullptr. Make sure the subsystem has FINISHED constructing.");
			return *sInstance;
		}
	};
}