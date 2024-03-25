#include "Precomp.h"
#include "World/World.h"

#include <stack>

#include "Components/ComponentFilter.h"
#include "Components/NameComponent.h"
#include "Meta/MetaProps.h"
#include "World/Registry.h"
#include "World/WorldViewport.h"
#include "Rendering/DebugRenderer.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"
#include "Assets/Level.h"
#include "Rendering/GPUWorld.h"

Engine::World::World(const bool beginPlayImmediately) :
	mRegistry(std::make_unique<Registry>(*this)),
	mPhysics(std::make_unique<Physics>(*this)),
	mViewport(std::make_unique<WorldViewport>(*this)),
	mGPUWorld(std::make_unique<GPUWorld>(*this))
{
	LOG(LogCore, Verbose, "World is awaiting begin play..");

	if (beginPlayImmediately)
	{
		BeginPlay();
	}
}

Engine::World::World(World&& other) noexcept :
	mRegistry(std::move(other.mRegistry)),
	mViewport(std::move(other.mViewport)),
	mPhysics(std::move(other.mPhysics)),
	mGPUWorld(std::move(other.mGPUWorld)),
	mLevelToTransitionTo(std::move(other.mLevelToTransitionTo)),
	mTime(other.mTime),
	mHasBegunPlay(other.mHasBegunPlay)
{
	mRegistry->mWorld = *this;
	mViewport->mWorld = *this;
	mPhysics->mWorld = *this;
	mGPUWorld->mWorld = *this;
}

Engine::World::~World()
{
	// Mightve been moved out
	if (mRegistry != nullptr)
	{
		mRegistry->Clear();
		mRegistry.reset();
		mViewport.reset();
	}
}

Engine::World& Engine::World::operator=(World&& other) noexcept
{
	mRegistry = std::move(other.mRegistry);
	mViewport = std::move(other.mViewport);
	mGPUWorld = std::move(other.mGPUWorld);
	mLevelToTransitionTo = std::move(other.mLevelToTransitionTo);

	mRegistry->mWorld = *this;
	mViewport->mWorld = *this;
	mPhysics->mWorld = *this;
	mGPUWorld->mWorld = *this;
	
	mTime = other.mTime;
	mHasBegunPlay = other.mHasBegunPlay;

	return *this;
}

void Engine::World::Tick(const float unscaledDeltaTime)
{
	PushWorld(*this);

	mTime.Step(unscaledDeltaTime);

	GetRegistry().UpdateSystems(unscaledDeltaTime);
	GetRegistry().RemovedDestroyed();

	if (GetNextLevel() != nullptr)
	{
		*this = GetNextLevel()->CreateWorld(true);
	}

	PopWorld();
}

void Engine::World::BeginPlay()
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

	for (auto&& [typeHash, storage] : mRegistry->Storage())
	{
		const MetaType* const metaType = MetaManager::Get().TryGetType(typeHash);

		if (metaType == nullptr)
		{
			continue;
		}

		const MetaFunc* const beginPlayEvent = TryGetEvent(*metaType, sBeginPlayEvent);

		if (beginPlayEvent == nullptr)
		{
			continue;
		}

		const bool isStatic = beginPlayEvent->GetProperties().Has(Props::sIsEventStaticTag);

		for (const entt::entity entity : storage)
		{
			// Tombstone check
			if (!storage.contains(entity))
			{
				continue;
			}

			if (isStatic)
			{
				beginPlayEvent->InvokeCheckedUnpacked(*this, entity);
			}
			else
			{
				MetaAny component{ *metaType, storage.value(entity), false };
				beginPlayEvent->InvokeCheckedUnpacked(component, *this, entity);
			}
		}
	}

}

void Engine::World::EndPlay()
{
	if (!HasBegunPlay())
	{
		LOG(LogCore, Warning, "Cannot end play, as the world has not begunplay");
	}

	LOG(LogCore, Verbose, "World has just ended play");

	mHasBegunPlay = false;
}

static inline std::stack<std::reference_wrapper<Engine::World>> sWorldStack{};

void Engine::World::PushWorld(World& world)
{
	sWorldStack.push(world);
}

void Engine::World::PopWorld(uint32 amountToPop)
{
	for (uint32 i = 0; i < amountToPop; i++)
	{
		sWorldStack.pop();
	}
}

Engine::World* Engine::World::TryGetWorldAtTopOfStack()
{
	if (sWorldStack.empty())
	{
		return nullptr;
	}
	return &sWorldStack.top().get();
}

void Engine::World::TransitionToLevel(const std::shared_ptr<const Level>& level)
{
	if (GetNextLevel() == nullptr)
	{
		mLevelToTransitionTo = level;
	}
}

namespace
{
	std::vector<entt::entity> FindAllEntitiesWithComponents(const Engine::World& world, const std::vector<Engine::ComponentFilter>& components, bool returnAfterFirstFound)
	{
		using namespace Engine;

		std::vector<std::reference_wrapper<const entt::sparse_set>> storages{};

		for (const ComponentFilter& component : components)
		{
			if (component == nullptr)
			{
				continue;
			}

			const entt::sparse_set* storage = world.GetRegistry().Storage(component.Get()->GetTypeId());

			if (storage == nullptr)
			{
				return {};
			}

			storages.push_back(*storage);
		}

		if (storages.empty())
		{
			return {};
		}

		std::sort(storages.begin(), storages.end(),
			[](const entt::sparse_set& lhs, const entt::sparse_set& rhs)
			{
				return lhs.size() < rhs.size();
			});

		std::vector<entt::entity> returnValue{};

		for (entt::entity entity : storages[0].get())
		{
			if (std::find_if(storages.begin(), storages.end(),
				[entity](const entt::sparse_set& storage)
				{
					return !storage.contains(entity);
				}) == storages.end())
			{
				returnValue.push_back(entity);

				if (returnAfterFirstFound)
				{
					return returnValue;
				}
			}
		}

		return returnValue;
	}

}

Engine::MetaType Engine::World::Reflect()
{
	MetaType type = MetaType{ MetaType::T<World>{}, "World" };
	type.GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([]
		{
			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			return world->HasBegunPlay();
		}, "HasBegunPlay").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

	type.AddFunc([]
		{
			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			return world->GetCurrentTimeScaled();
		}, "GetCurrentTimeScaled").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

	type.AddFunc([]
		{
			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			return world->GetCurrentTimeReal();
		}, "GetCurrentTimeReal").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

	type.AddFunc([]
		{
			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			return world->GetTimeScale(); }, "GetTimeScale").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

	type.AddFunc([](float timescale)
		{
			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			world->SetTimeScale(timescale);
		}, "SetTimeScale", MetaFunc::ExplicitParams<float>{}, "TimeScale").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([]
		{
			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			return world->IsPaused();
		}, "IsPaused").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

	type.AddFunc([]
		{
			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			world->Pause();
		}, "Pause").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([]
		{
			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			world->Unpause();
		}, "Unpause").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](bool isPaused)
		{
			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			world->SetIsPaused(isPaused);
		}, "SetIsPaused", MetaFunc::ExplicitParams<bool>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([]
		{
			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			return world->HasBegunPlay();
		}, "HasBegunPlay").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

	type.AddFunc([](const std::shared_ptr<const Level>& level)
		{
			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			world->TransitionToLevel(level);
		},
		"TransitionToLevel", 
		MetaFunc::ExplicitParams<const std::shared_ptr<const Level>&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([]
		{
			const World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			std::vector<entt::entity> returnValue{};

			const auto entityStorage = world->GetRegistry().Storage<entt::entity>();

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

	type.AddFunc([]
		{
			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			world->GetRegistry().Clear();
		}, "Clear").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([]
		{
			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			return world->GetRegistry().Create();
		}, "Spawn entity").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const std::shared_ptr<const Prefab>& prefab)
		{
			if (prefab == nullptr)
			{
				LOG(LogWorld, Warning, "Attempted to spawn NULL prefab.");
			}

			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			return world->GetRegistry().CreateFromPrefab(*prefab);
		}, "Spawn prefab", MetaFunc::ExplicitParams<const std::shared_ptr<const Prefab>&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const entt::entity& entity, bool destroyChildren)
		{
			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			world->GetRegistry().Destroy(entity, destroyChildren);
		}, "Destroy entity", MetaFunc::ExplicitParams<const entt::entity&, bool>{}, "Entity", "DestroyChildren").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const std::string& name) -> entt::entity
		{
			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			const auto view = world->GetRegistry().View<const NameComponent>();

			for (auto [entity, nameComponent] : view.each())
			{
				if (nameComponent.mName == name)
				{
					return entity;
				}
			}

			return entt::null;
		}, "Find entity with name", MetaFunc::ExplicitParams<const std::string&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const std::string& name)
		{
			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			std::vector<entt::entity> returnValue{};
			const auto view = world->GetRegistry().View<const NameComponent>();

			for (auto [entity, nameComponent] : view.each())
			{
				if (nameComponent.mName == name)
				{
					returnValue.push_back(entity);
				}
			}

			return returnValue;
		}, "Find all entities with name", MetaFunc::ExplicitParams<const std::string&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const ComponentFilter& component)
		{
			const World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			const std::vector<entt::entity> entities = FindAllEntitiesWithComponents(*world, { component }, true);
			return entities.empty() ? entt::null : entities[0];
		}, "Find entity with component", MetaFunc::ExplicitParams<const ComponentFilter&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const ComponentFilter& component)
		{
			const World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			return FindAllEntitiesWithComponents(*world, { component }, false);
		}, "Find all entities with component", MetaFunc::ExplicitParams<const ComponentFilter&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const std::vector<ComponentFilter>& components)
		{
			const World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			const std::vector<entt::entity> entities = FindAllEntitiesWithComponents(*world, components, true);
			return entities.empty() ? entt::null : entities[0];
		}, "Find entity with components", MetaFunc::ExplicitParams<const std::vector<ComponentFilter>&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const std::vector<ComponentFilter>& components)
		{
			const World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			return FindAllEntitiesWithComponents(*world, components, false);
		}, "Find all entities with components", MetaFunc::ExplicitParams<const std::vector<ComponentFilter>&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](glm::vec2 screenPosition, float distanceFromCamera)
		{
			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			return world->GetViewport().ScreenToWorld(screenPosition, distanceFromCamera);
		}, "ScreenToWorld", MetaFunc::ExplicitParams<glm::vec2, float>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

	return type;
}
