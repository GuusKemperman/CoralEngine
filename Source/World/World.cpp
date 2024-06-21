#include "Precomp.h"
#include "World/World.h"

#include <stack>
#include "entt/entity/runtime_view.hpp"

#include "Core/Device.h"
#include "Components/ComponentFilter.h"
#include "Components/NameComponent.h"
#include "Meta/MetaProps.h"
#include "World/Registry.h"
#include "World/WorldViewport.h"
#include "World/Physics.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"
#include "Assets/Level.h"
#include "Rendering/GPUWorld.h"

CE::World::World(const bool beginPlayImmediately) :
	mRegistry(std::make_unique<Registry>(*this)),
	mViewport(std::make_unique<WorldViewport>(*this)),
	mPhysics(std::make_unique<Physics>(*this))
{
	if (beginPlayImmediately)
	{
		BeginPlay();
	}
}

CE::World::World(World&& other) noexcept :
	mTime(other.mTime),
	mHasBegunPlay(other.mHasBegunPlay),
	mRegistry(std::move(other.mRegistry)),
	mViewport(std::move(other.mViewport)),
	mGPUWorld(std::move(other.mGPUWorld)),
	mPhysics(std::move(other.mPhysics)),
	mLevelToTransitionTo(std::move(other.mLevelToTransitionTo))
{
	mRegistry->mWorld = *this;
	mViewport->mWorld = *this;
	mPhysics->mWorld = *this;

	if (mGPUWorld != nullptr)
	{
		mGPUWorld->mWorld = *this;
	}
}

CE::World::~World()
{
	// Mightve been moved out
	if (mRegistry != nullptr)
	{
		mRegistry->Clear();
		mRegistry.reset();
		mViewport.reset();
	}
}

CE::World& CE::World::operator=(World&& other) noexcept
{
	if (&other == this)
	{
		return *this;
	}

	mRegistry = std::move(other.mRegistry);
	mViewport = std::move(other.mViewport);
	mGPUWorld = std::move(other.mGPUWorld);
	mPhysics = std::move(other.mPhysics);
	mLevelToTransitionTo = std::move(other.mLevelToTransitionTo);

	mRegistry->mWorld = *this;
	mViewport->mWorld = *this;
	mPhysics->mWorld = *this;

	if (mGPUWorld != nullptr)
	{
		mGPUWorld->mWorld = *this;
	}

	mTime = other.mTime;
	mHasBegunPlay = other.mHasBegunPlay;

	return *this;
}

void CE::World::Tick(const float unscaledDeltaTime)
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

CE::GPUWorld& CE::World::GetGPUWorld() const
{
	ASSERT_LOG(!Device::IsHeadless(), "Cannot access GPUWorld when device is running in headless mode. Check using Device::IsHeadless.");

	if (mGPUWorld == nullptr)
	{
		// The GPU buffers are only allocated when GetGPUWorld is called.
		// Otherwise, we allocate a looot of resources that are only
		// freed at the end of the frame, and if we create a lot of worlds
		// in one frame (such as with unit tests), then we run out of memory.
		// hence, the const_cast
		const_cast<World&>(*this).mGPUWorld = std::make_unique<GPUWorld>(*this);
	}

	return *mGPUWorld;
}

static std::mutex sStackMutex{};
static std::vector<std::pair<std::thread::id, std::stack<std::reference_wrapper<CE::World>>>> sWorldStacks{};

void CE::World::PushWorld(World& world)
{
	const std::thread::id curr = std::this_thread::get_id();

	sStackMutex.lock();

	if (sWorldStacks.size() >= 31)
	{
		// We always keep the main thread's stack in there
		sWorldStacks.erase(std::remove_if(sWorldStacks.begin() + 1, sWorldStacks.end(),
			[](const std::pair<std::thread::id, std::stack<std::reference_wrapper<World>>>& stack)
			{
				return stack.second.empty();
			}), sWorldStacks.end());
	}

	const auto it = std::find_if(sWorldStacks.begin(), sWorldStacks.end(),
		[curr](const std::pair<std::thread::id, std::stack<std::reference_wrapper<World>>>& stack)
		{
			return stack.first == curr;
		});

	if (it == sWorldStacks.end())
	{
		sWorldStacks.emplace_back(curr, std::stack<std::reference_wrapper<World>>{}).second.push(world);
	}
	else
	{
		it->second.push(world);
	}

	sStackMutex.unlock();
}

void CE::World::PopWorld(uint32 amountToPop)
{
	const std::thread::id curr = std::this_thread::get_id();

	sStackMutex.lock();

	const auto it = std::find_if(sWorldStacks.begin(), sWorldStacks.end(),
		[curr](const std::pair<std::thread::id, std::stack<std::reference_wrapper<World>>>& stack)
		{
			return stack.first == curr;
		});

	for (uint32 i = 0; i < amountToPop; i++)
	{
		it->second.pop();
	}
	sStackMutex.unlock();
}

CE::World* CE::World::TryGetWorldAtTopOfStack()
{
	const std::thread::id curr = std::this_thread::get_id();

	sStackMutex.lock();

	const auto it = std::find_if(sWorldStacks.begin(), sWorldStacks.end(),
		[curr](const std::pair<std::thread::id, std::stack<std::reference_wrapper<World>>>& stack)
		{
			return stack.first == curr;
		});

	if (it == sWorldStacks.end()
		|| it->second.empty())
	{
		sStackMutex.unlock();
		return nullptr;
	}

	World* world = &it->second.top().get();
	sStackMutex.unlock();
	return world;
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
	entt::runtime_view FindAllEntitiesWithComponents(const CE::World& world, const CE::Span<const CE::ComponentFilter>& components)
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

	type.AddFunc([]
		{
			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			world->RequestEndplay();
		}, "RequestEndPlay").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const AssetHandle<Level>& level)
		{
			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			world->TransitionToLevel(level);
		},
		"TransitionToLevel", 
		MetaFunc::ExplicitParams<const AssetHandle<Level>&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

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

	type.AddFunc([](const AssetHandle<Prefab>& prefab)
		{
			if (prefab == nullptr)
			{
				LOG(LogWorld, Warning, "Attempted to spawn NULL prefab.");
			}

			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			return world->GetRegistry().CreateFromPrefab(*prefab);
		}, "Spawn prefab", MetaFunc::ExplicitParams<const AssetHandle<Prefab>&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const AssetHandle<Prefab>& prefab, const glm::vec3 position, const glm::quat orientation, const glm::vec3 scale, TransformComponent* parent) -> entt::entity
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

			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			return world->GetRegistry().CreateFromPrefab(*prefab, entt::null, &position, &orientation, &scale, parent);
		}, "Spawn prefab at", MetaFunc::ExplicitParams<const AssetHandle<Prefab>&, glm::vec3, glm::quat, glm::vec3, TransformComponent*>{},
			"Prefab", "LocalPosition", "LocalOrientation", "LocalScale", "Parent").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

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

	type.AddFunc([](const ComponentFilter& component) -> entt::entity
		{
			const World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			const entt::runtime_view view = FindAllEntitiesWithComponents(*world, { &component, 1 });

			if (view.begin() == view.end())
			{
				return entt::null;
			}
			return *view.begin();
		}, "Find entity with component", MetaFunc::ExplicitParams<const ComponentFilter&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const ComponentFilter& component)
		{
			const World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			const entt::runtime_view view = FindAllEntitiesWithComponents(*world, { &component, 1 });

			std::vector<entt::entity> returnValue{};
			returnValue.reserve(view.size_hint());
			returnValue.insert(returnValue.end(), view.begin(), view.end());

			return returnValue;
		}, "Find all entities with component", MetaFunc::ExplicitParams<const ComponentFilter&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const std::vector<ComponentFilter>& components) -> entt::entity
		{
			const World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			const entt::runtime_view view = FindAllEntitiesWithComponents(*world, components);

			if (view.begin() == view.end())
			{
				return entt::null;
			}
			return *view.begin();
		}, "Find entity with components", MetaFunc::ExplicitParams<const std::vector<ComponentFilter>&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const std::vector<ComponentFilter>& components)
		{
			const World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			const entt::runtime_view view = FindAllEntitiesWithComponents(*world, components);

			std::vector<entt::entity> returnValue{};
			returnValue.reserve(view.size_hint());
			returnValue.insert(returnValue.end(), view.begin(), view.end());

			return returnValue;
		}, "Find all entities with components", MetaFunc::ExplicitParams<const std::vector<ComponentFilter>&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](glm::vec2 screenPosition, float distanceFromCamera)
		{
			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			return world->GetViewport().ScreenToWorld(screenPosition, distanceFromCamera);
		}, "ScreenToWorld", MetaFunc::ExplicitParams<glm::vec2, float>{}, "Screen position", "Distance from camera").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

	type.AddFunc([](glm::vec2 screenPosition, float planeHeight)
		{
			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			return world->GetViewport().ScreenToWorldPlane(screenPosition, planeHeight);
		}, "ScreenToWorldPlane", MetaFunc::ExplicitParams<glm::vec2, float>{}, "Screen position", "Plane height").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

	type.AddFunc([]()
		{
			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			return world->GetScaledDeltaTime();
		}, "GetScaledDeltaTime", MetaFunc::ExplicitParams<>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

	type.AddFunc([]()
		{
			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			return world->GetRealDeltaTime();
		}, "GetRealDeltaTime", MetaFunc::ExplicitParams<>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

	return type;
}
