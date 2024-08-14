#include "Precomp.h"
#include "World/World.h"

#include <stack>
#include "entt/entity/runtime_view.hpp"

#include "Utilities/ComponentFilter.h"
#include "Components/NameComponent.h"
#include "Meta/MetaProps.h"
#include "World/Registry.h"
#include "World/WorldViewport.h"
#include "World/Physics.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"
#include "Assets/Level.h"
#include "Components/CameraComponent.h"
#include "Core/Device.h"
#include "Core/Renderer.h"
#include "Rendering/FrameBuffer.h"
#include "World/EventManager.h"

CE::World::World(const bool beginPlayImmediately) :
	mRegistry(std::make_unique<Registry>(*this)),
	mViewport(std::make_unique<WorldViewport>(*this)),
	mPhysics(std::make_unique<Physics>(*this)),
	mEventManager(std::make_unique<EventManager>(*this)),
	mRenderCommandQueue(Device::IsHeadless() ? nullptr : Renderer::Get().CreateCommandQueue())
{
	if (beginPlayImmediately)
	{
		BeginPlay();
	}
}

CE::World::~World()
{
	mRegistry->Clear();
	mRegistry.reset();
	mViewport.reset();
}

void CE::World::Tick(const float unscaledDeltaTime)
{
	mTime.Step(unscaledDeltaTime);

	GetRegistry().UpdateSystems(unscaledDeltaTime);
	GetRegistry().RemovedDestroyed();

	const AssetHandle<Level> nextLevel = GetNextLevel();

	if (nextLevel != nullptr)
	{
		const bool hasBegunPlay = mHasBegunPlay;

		World& self = *this;
		self.~World();
		new (this)World(false);

		nextLevel->LoadIntoWorld(self);

		if (hasBegunPlay)
		{
			BeginPlay();
		}
	}
}

void CE::World::Render(glm::vec2 renderPos, FrameBuffer* renderTarget)
{
	mViewport->UpdateSize(renderTarget == nullptr ? Device::Get().GetDisplaySize() : renderTarget->mSize, renderPos);
	mRegistry->RenderSystems(*mRenderCommandQueue);

	const entt::entity cameraEntity = CameraComponent::GetSelected(*this);

	if (cameraEntity == entt::null)
	{
		LOG(LogTemp, Message, "No camera to render to");
		return;
	}

	const CameraComponent& camera = mRegistry->Get<const CameraComponent>(cameraEntity);
	Renderer::Get().SetRenderTarget(*mRenderCommandQueue, camera.mView, camera.mProjection, renderTarget);
}

void CE::World::BeginPlay()
{
	if (HasBegunPlay())
	{
		LOG(LogCore, Warning, "Called BeginPlay twice");
		return;
	}

	mHasBegunPlay = true;

	// Reset the total time elapsed, deltaTime, etc
	mTime = {};

	LOG(LogCore, Verbose, "World will begin play");
	mRegistry->BeginPlay();
}

void CE::World::EndPlay()
{
	if (!HasBegunPlay())
	{
		LOG(LogCore, Warning, "Cannot end play, as the world has not begunplay");
	}

	LOG(LogCore, Verbose, "World has just ended play");

	mHasBegunPlay = false;
}

CE::Registry& CE::World::GetRegistry()
{
	return *mRegistry;
}

const CE::Registry& CE::World::GetRegistry() const
{
	return *mRegistry;
}

CE::Physics& CE::World::GetPhysics()
{
	return *mPhysics;
}

const CE::Physics& CE::World::GetPhysics() const
{
	return *mPhysics;
}

CE::EventManager& CE::World::GetEventManager()
{
	return *mEventManager;
}

const CE::EventManager& CE::World::GetEventManager() const
{
	return *mEventManager;
}

CE::WorldViewport& CE::World::GetViewport()
{
	return *mViewport;
}

const CE::WorldViewport& CE::World::GetViewport() const
{
	return *mViewport;
}

CE::RenderCommandQueue& CE::World::GetRenderCommandQueue()
{
	return *mRenderCommandQueue;
}

const CE::RenderCommandQueue& CE::World::GetRenderCommandQueue() const
{
	return *mRenderCommandQueue;
}

bool CE::World::HasBegunPlay() const
{
	return mHasBegunPlay;
}

bool CE::World::HasRequestedEndPlay() const
{
	return mHasEndPlayBeenRequested;
}

float CE::World::GetCurrentTimeScaled() const
{
	return mTime.mScaledTotalTimeElapsed;
}

float CE::World::GetCurrentTimeReal() const
{
	return mTime.mRealTotalTimeElapsed;
}

float CE::World::GetTimeScale() const
{
	return mTime.GetTimeScale();
}

void CE::World::SetTimeScale(float timeScale)
{
	mTime.mTimescale = timeScale;
}

float CE::World::GetRealDeltaTime() const
{
	return mTime.mRealDeltaTime;
}

float CE::World::GetScaledDeltaTime() const
{
	return mTime.mScaledDeltaTime;
}

bool CE::World::IsPaused() const
{
	return mTime.mIsPaused;
}

void CE::World::Pause()
{
	mTime.mIsPaused = true;
}

void CE::World::Unpause()
{
	mTime.mIsPaused = false;
}

void CE::World::SetIsPaused(bool isPaused)
{
	mTime.mIsPaused = isPaused;
}

void CE::World::RequestEndplay()
{
	mHasEndPlayBeenRequested = true;
}

const CE::AssetHandle<CE::Level>& CE::World::GetNextLevel() const
{
	return mLevelToTransitionTo;
}

void CE::World::TransitionToLevel(const AssetHandle<Level>& level)
{
	if (GetNextLevel() == nullptr)
	{
		mLevelToTransitionTo = level;
	}
}

namespace
{
	entt::runtime_view FindAllEntitiesWithComponents(const CE::World& world, const std::span<const CE::ComponentFilter>& components)
	{
		entt::runtime_view view{};

		for (const CE::ComponentFilter& component : components)
		{
			if (component == nullptr)
			{
				continue;
			}

			const entt::sparse_set* storage = world.GetRegistry().Storage(component.Get()->GetTypeId());

			if (storage != nullptr)
			{
				// Const_cast is fine, we are not actually modifying the
				// view, we just want to collect a list of entities.
				view.iterate(const_cast<entt::sparse_set&>(*storage));
			}
		}

		return view;
	}
}

CE::MetaType CE::World::Reflect()
{
	MetaType type = MetaType{ MetaType::T<World>{}, "World" };
	type.GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const World& world)
		{
			return world.HasBegunPlay();
		}, "HasBegunPlay").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

	type.AddFunc([](const World& world)
		{
			return world.GetCurrentTimeScaled();
		}, "GetCurrentTimeScaled").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

	type.AddFunc([](const World& world)
		{
			return world.GetCurrentTimeReal();
		}, "GetCurrentTimeReal").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

	type.AddFunc([](const World& world)
		{
			return world.GetTimeScale();
		}, "GetTimeScale").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

	type.AddFunc([](World& world, float timescale)
		{
			world.SetTimeScale(timescale);
		}, "SetTimeScale", "TimeScale").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const World& world)
		{
			return world.IsPaused();
		}, "IsPaused").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

	type.AddFunc([](World& world)
		{
			world.Pause();
		}, "Pause").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](World& world)
		{
			world.Unpause();
		}, "Unpause").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](World& world, bool isPaused)
		{
			world.SetIsPaused(isPaused);
		}, "SetIsPaused").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const World& world)
		{
			return world.HasBegunPlay();
		}, "HasBegunPlay").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

	type.AddFunc([](World& world)
		{
			world.RequestEndplay();
		}, "RequestEndPlay").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](World& world, const AssetHandle<Level>& level)
		{
			world.TransitionToLevel(level);
		},
		"TransitionToLevel").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const World& world)
		{
			std::vector<entt::entity> returnValue{};

			const auto entityStorage = world.GetRegistry().Storage<entt::entity>();

			if (entityStorage == nullptr)
			{
				return returnValue;
			}

			returnValue.reserve(entityStorage->size());

			for (const auto [entity] : entityStorage->each())
			{
				returnValue.push_back(entity);
			}

			return returnValue;
		}, "Get all entities").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

	type.AddFunc([](World& world)
		{
			world.GetRegistry().Clear();
		}, "Clear").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](World& world)
		{
			return world.GetRegistry().Create();
		}, "Spawn entity").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](World& world, const AssetHandle<Prefab>& prefab)
		{
			if (prefab == nullptr)
			{
				LOG(LogWorld, Warning, "Attempted to spawn NULL prefab.");
			}

			return world.GetRegistry().CreateFromPrefab(*prefab);
		}, "Spawn prefab").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](World& world, const AssetHandle<Prefab>& prefab, const glm::vec3 position, const glm::quat orientation, const glm::vec3 scale, TransformComponent* parent) -> entt::entity
		{
			if (prefab == nullptr)
			{
				LOG(LogWorld, Warning, "Attempted to spawn NULL prefab.");
				return entt::null;
			}

			if (scale == glm::vec3{})
			{
				LOG(LogWorld, Warning, "Spawning prefab {} with a scale of (0, 0, 0), may not be intended.", prefab.GetMetaData().GetName());
			}

			return world.GetRegistry().CreateFromPrefab(*prefab, entt::null, &position, &orientation, &scale, parent);
		}, "Spawn prefab at",
			"Prefab", "LocalPosition", "LocalOrientation", "LocalScale", "Parent").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](World& world, const entt::entity& entity, bool destroyChildren)
		{
			world.GetRegistry().Destroy(entity, destroyChildren);
		}, "Destroy entity", "Entity", "DestroyChildren").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const World& world, const std::string& name) -> entt::entity
		{
			const auto view = world.GetRegistry().View<const NameComponent>();

			for (auto [entity, nameComponent] : view.each())
			{
				if (nameComponent.mName == name)
				{
					return entity;
				}
			}

			return entt::null;
		}, "Find entity with name").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const World& world, const std::string& name)
		{
			std::vector<entt::entity> returnValue{};
			const auto view = world.GetRegistry().View<const NameComponent>();

			for (auto [entity, nameComponent] : view.each())
			{
				if (nameComponent.mName == name)
				{
					returnValue.push_back(entity);
				}
			}

			return returnValue;
		}, "Find all entities with name").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const World& world, const ComponentFilter& component) -> entt::entity
		{
			const entt::runtime_view view = FindAllEntitiesWithComponents(world, { &component, 1 });

			if (view.begin() == view.end())
			{
				return entt::null;
			}
			return *view.begin();
		}, "Find entity with component").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const World& world, const ComponentFilter& component)
		{
			const entt::runtime_view view = FindAllEntitiesWithComponents(world, { &component, 1 });

			std::vector<entt::entity> returnValue{};
			returnValue.reserve(view.size_hint());
			returnValue.insert(returnValue.end(), view.begin(), view.end());

			return returnValue;
		}, "Find all entities with component").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const World& world, const std::vector<ComponentFilter>& components) -> entt::entity
		{
			const entt::runtime_view view = FindAllEntitiesWithComponents(world, components);

			if (view.begin() == view.end())
			{
				return entt::null;
			}
			return *view.begin();
		}, "Find entity with components").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const World& world, const std::vector<ComponentFilter>& components)
		{
			const entt::runtime_view view = FindAllEntitiesWithComponents(world, components);

			std::vector<entt::entity> returnValue{};
			returnValue.reserve(view.size_hint());
			returnValue.insert(returnValue.end(), view.begin(), view.end());

			return returnValue;
		}, "Find all entities with components").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const World& world, glm::vec2 screenPosition, float distanceFromCamera)
		{
			return world.GetViewport().ScreenToWorld(screenPosition, distanceFromCamera);
		}, "ScreenToWorld", "Screen position", "Distance from camera").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

	type.AddFunc([](const World& world, glm::vec2 screenPosition, float planeHeight)
		{
			return world.GetViewport().ScreenToWorldPlane(screenPosition, planeHeight);
		}, "ScreenToWorldPlane", "Screen position", "Plane height").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

	type.AddFunc([](const World& world)
		{
			return world.GetScaledDeltaTime();
		}, "GetScaledDeltaTime").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

	type.AddFunc([](const World& world)
		{
			return world.GetRealDeltaTime();
		}, "GetRealDeltaTime").GetProperties().Add(Props::sIsScriptableTag);

	return type;
}
