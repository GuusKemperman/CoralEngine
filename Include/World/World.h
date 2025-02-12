#pragma once
#include "Assets/Core/AssetHandle.h"
#include "BasicDataTypes/ScalableTimer.h"
#include "Meta/MetaReflect.h"
#include "Systems/System.h"

namespace CE
{
	struct RenderCommandQueue;
	class FrameBuffer;

	class Level;
	class Registry;
	class WorldViewport;
	class Physics;
	class EventManager;

	class World
	{
	public:
		World(bool beginPlayImmediately);

		World(World&&) = delete;
		World(const World&) = delete;

		World& operator=(World&&) noexcept = delete;
		World& operator=(const World&) = delete;

		~World();

		void Tick(float deltaTime);

		void Render(glm::vec2 viewportPos, FrameBuffer* renderTarget = nullptr);

		void BeginPlay();
		void EndPlay();

		Registry& GetRegistry();
		const Registry& GetRegistry() const;

		template<typename T>
		T& AddSystem();

		// May return nullptr if construction fails,
		// or if the type does not derive from System.
		System* AddSystem(const MetaType& systemType);

		template<typename T>
		T* TryGetSystem();

		template<typename T>
		const T* TryGetSystem() const;

		System* TryGetSystem(TypeId typeId);

		const System* TryGetSystem(TypeId typeId) const;

		Physics& GetPhysics();
		const Physics& GetPhysics() const;

		EventManager& GetEventManager();
		const EventManager& GetEventManager() const;

		WorldViewport& GetViewport();
		const WorldViewport& GetViewport() const;

		RenderCommandQueue& GetRenderCommandQueue();
		const RenderCommandQueue& GetRenderCommandQueue() const;

		bool HasBegunPlay() const;

		bool HasRequestedEndPlay() const;

		// In seconds
		float GetCurrentTimeScaled() const;

		// In seconds
		float GetCurrentTimeReal() const;

		float GetTimeScale() const;

		void SetTimeScale(float timeScale);

		float GetRealDeltaTime() const;

		float GetScaledDeltaTime() const;

		bool IsPaused() const;

		void Pause();

		void Unpause();

		void SetIsPaused(bool isPaused);

		void RequestEndplay();

		/**
		 * \brief Will request a transition to a different level.
		 *
		 * Transitioning to a different level involves the destruction of the current world, and the
		 * creation of a new one. This is done at the end of the World::Tick. All systems will be restarted.
		 *
		 * \param level The level to transition to.
		 */
		void TransitionToLevel(const AssetHandle<Level>& level);

		const AssetHandle<Level>& GetNextLevel() const;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(World);

		struct SingleTick
		{
			SingleTick(System& system, float deltaTime = 0.0f) : mSystem(system), mDeltaTime(deltaTime) {};
			std::reference_wrapper<System> mSystem;
			float mDeltaTime{};
		};
		std::vector<SingleTick> GetSortedSystemsToUpdate(float deltaTime);

		System& AddSystem(std::unique_ptr<System, InPlaceDeleter<System, true>> system, TypeId typeId);

		struct InternalSystem
		{
			InternalSystem(std::unique_ptr<System, InPlaceDeleter<System, true>> system, SystemStaticTraits traits) :
				mSystem(std::move(system)),
				mTraits(traits) {
			}

			// Systems are created using the runtime reflection system,
			// which uses placement new for the constructing of objects.
			// Hence, the custom deleter
			std::unique_ptr<System, InPlaceDeleter<System, true>> mSystem{};
			SystemStaticTraits mTraits{};
		};

		struct FixedTickSystem :
			InternalSystem
		{
			FixedTickSystem(std::unique_ptr<System, InPlaceDeleter<System, true>> system, SystemStaticTraits traits, float timeOfNextStep) :
				InternalSystem(std::move(system), traits),
				mTimeOfNextStep(timeOfNextStep) {
			}
			float mTimeOfNextStep{};
		};
		std::vector<FixedTickSystem> mFixedTickSystems{};
		std::vector<InternalSystem> mNonFixedSystems{};
		std::unordered_map<TypeId, std::reference_wrapper<System>> mSystemLookUp{};

		ScalableTimer mTime{};
		bool mHasBegunPlay{};

		std::unique_ptr<Registry> mRegistry{};
		std::unique_ptr<WorldViewport> mViewport{};
		std::unique_ptr<Physics> mPhysics{};
		std::unique_ptr<EventManager> mEventManager{};
		std::shared_ptr<RenderCommandQueue> mRenderCommandQueue{};

		AssetHandle<Level> mLevelToTransitionTo{};
		bool mHasEndPlayBeenRequested = false;
	};

	template <typename T>
	T& World::AddSystem()
	{
		if (T* existingSystem = TryGetSystem<T>(); existingSystem != nullptr)
		{
			return *existingSystem;
		}

		return static_cast<T&>(AddSystem(MakeUniqueInPlace<T, System>(), MakeStrippedTypeId<T>()));
	}

	template <typename T>
	T* World::TryGetSystem()
	{
		return static_cast<T*>(TryGetSystem(MakeStrippedTypeId<T>()));
	}

	template <typename T>
	const T* World::TryGetSystem() const
	{
		return static_cast<const T*>(TryGetSystem(MakeStrippedTypeId<T>()));
	}
}
