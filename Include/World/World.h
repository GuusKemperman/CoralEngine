#pragma once
#include "Assets/Core/AssetHandle.h"
#include "BasicDataTypes/ScalableTimer.h"
#include "Meta/MetaReflect.h"

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
		World(World&& other) noexcept;
		World(const World&) = delete;

		~World();

		World& operator=(World&& other) noexcept;
		World& operator=(const World&) = delete;

		void Tick(float deltaTime);
		void Render(FrameBuffer* renderTarget = nullptr);

		void BeginPlay();
		void EndPlay();

		Registry& GetRegistry();
		const Registry& GetRegistry() const;

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

		static void PushWorld(World& world);
		static void PopWorld();

		static World* TryGetWorldAtTopOfStack();

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
}
