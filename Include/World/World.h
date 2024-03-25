#pragma once
#include "BasicDataTypes/ScalableTimer.h"
#include "Meta/MetaReflect.h"

namespace Engine
{
	class Level;
	class Registry;
	class WorldViewport;
	class GPUWorld;
	class DebugRenderer;
	class BinaryGSONObject;

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

		void BeginPlay();
		void EndPlay();

		Registry& GetRegistry() { ASSERT(mRegistry != nullptr); return *mRegistry; };
		const Registry& GetRegistry() const { ASSERT(mRegistry != nullptr); return *mRegistry; };

		WorldViewport& GetViewport() { ASSERT(mViewport != nullptr); return *mViewport; };
		const WorldViewport& GetViewport() const { ASSERT(mViewport != nullptr); return *mViewport; };

		GPUWorld& GetGPUWorld() const { return *mGPUWorld; };

		bool HasBegunPlay() const { return mHasBegunPlay; }

		// In seconds
		float GetCurrentTimeScaled() const { return mTime.mScaledTotalTimeElapsed; }

		// In seconds
		float GetCurrentTimeReal() const { return mTime.mRealTotalTimeElapsed; }

		float GetTimeScale() const { return mTime.GetTimeScale(); }

		void SetTimeScale(float timeScale) { mTime.mTimescale = timeScale; }

		bool IsPaused() const { return mTime.mIsPaused; }

		void Pause() { mTime.mIsPaused = true; }

		void Unpause() { mTime.mIsPaused = false; }

		void SetIsPaused(bool isPaused) { mTime.mIsPaused = isPaused; }

		static void PushWorld(World& world);
		static void PopWorld(uint32 amountToPop = 1);

		static World* TryGetWorldAtTopOfStack();

		/**
		 * \brief Will request a transition to a different level.
		 *
		 * Transitioning to a different level involves the destruction of the current world, and the
		 * creation of a new one. This is done at the end of the World::Tick. All systems will be restarted.
		 *
		 * \param level The level to transition to.
		 */
		void TransitionToLevel(const std::shared_ptr<const Level>& level);

		const std::shared_ptr<const Level>& GetNextLevel() const { return mLevelToTransitionTo; }

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(World);

		std::unique_ptr<Registry> mRegistry{};
		std::unique_ptr<WorldViewport> mViewport{};
		std::unique_ptr<GPUWorld> mGPUWorld{};

		std::shared_ptr<const Level> mLevelToTransitionTo{};

		ScalableTimer mTime{};

		bool mHasBegunPlay{};
	};
}
