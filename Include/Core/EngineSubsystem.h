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
		static Derived& StartUp(Args&&... args);

		static void ShutDown();

	public:
		[[nodiscard]] static Derived& Get();
	};

	template <class Derived>
	template <typename ... Args>
	Derived& EngineSubsystem<Derived>::StartUp(Args&&... args)
	{
		assert(sInstance == nullptr);
		sInstance = new Derived(std::forward<Args>(args)...);
		sInstance->PostConstruct();
		return *sInstance;
	}

	template <class Derived>
	void EngineSubsystem<Derived>::ShutDown()
	{
		delete sInstance;
	}

	template <class Derived>
	Derived& EngineSubsystem<Derived>::Get()
	{
		assert(sInstance != nullptr && "Instance was nullptr. Make sure the subsystem has FINISHED constructing.");
		return *sInstance;
	}
}
