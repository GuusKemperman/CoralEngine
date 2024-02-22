#include "Precomp.h"
#include "World/World.h"

#include <stack>

#include "Meta/MetaProps.h"
#include "World/Registry.h"
#include "World/Archiver.h"
#include "World/WorldRenderer.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"

Engine::World::World(const bool beginPlayImmediately)
{
	mRenderer = std::make_unique<WorldRenderer>(*this);
	mRegistry = std::make_unique<Registry>(*this);

	LOG_TRIVIAL(LogCore, Verbose, "World is awaiting begin play..");

	if (beginPlayImmediately)
	{
		BeginPlay();
	}
}

Engine::World::World(World&& other) noexcept :
	mRegistry(std::move(other.mRegistry)),
	mRenderer(std::move(other.mRenderer)),
	mTime(other.mTime),
	mHasBegunPlay(other.mHasBegunPlay)
{
	mRegistry->mWorld = *this;
	mRenderer->mWorld = *this;
}

Engine::World::~World()
{
	mRegistry.reset();
	mRenderer.reset();
}

Engine::World& Engine::World::operator=(World&& other) noexcept
{
	mRegistry = std::move(other.mRegistry);
	mRenderer = std::move(other.mRenderer);
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

	PopWorld();
}

void Engine::World::BeginPlay()
{
	if (HasBegunPlay())
	{
		LOG_TRIVIAL(LogCore, Warning, "Called BeginPlay twice");
		return;
	}

	mHasBegunPlay = true;

	// Reset the total time elapsed, deltaTime, etc
	mTime = {};
	LOG_TRIVIAL(LogCore, Verbose, "World has just begun play");
}

void Engine::World::EndPlay()
{
	if (!HasBegunPlay())
	{
		LOG_TRIVIAL(LogCore, Warning, "Cannot end play, as the world has not begunplay");
	}

	LOG_TRIVIAL(LogCore, Verbose, "World has just ended play");

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

	// We hide the distinction between the registry and the world from the designers
	type.AddFunc([]
		{
			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			return world->GetRegistry().GetAllEntities();
		}, "Get all entities").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

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
				LOG_TRIVIAL(LogWorld, Warning, "Attempted to spawn NULL prefab.");
			}

			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			return world->GetRegistry().CreateFromPrefab(*prefab);
		}, "Spawn prefab", MetaFunc::ExplicitParams<const std::shared_ptr<const Prefab>&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const entt::entity& entity)
		{
			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			world->GetRegistry().DestroyAlongWithChildren(entity);
		}, "Destroy entity", MetaFunc::ExplicitParams<const entt::entity&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const std::string& name)
		{
			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			return world->GetRegistry().FindEntityWithName(name);
		}, "Find entity with name", MetaFunc::ExplicitParams<const std::string&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const std::string& name)
		{
			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			return world->GetRegistry().FindAllEntitiesWithName(name);
		}, "Find all entities with name", MetaFunc::ExplicitParams<const std::string&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const ComponentFilter& component)
		{
			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			return world->GetRegistry().FindEntityWithComponent(component);
		}, "Find entity with component", MetaFunc::ExplicitParams<const ComponentFilter&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const ComponentFilter& component)
		{
			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			return world->GetRegistry().FindAllEntitiesWithComponent(component);
		}, "Find all entities with component", MetaFunc::ExplicitParams<const ComponentFilter&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const std::vector<ComponentFilter>& components)
		{
			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			return world->GetRegistry().FindEntityWithComponents(components);
		}, "Find entity with components", MetaFunc::ExplicitParams<const std::vector<ComponentFilter>&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const std::vector<ComponentFilter>& components)
		{
			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			return world->GetRegistry().FindAllEntitiesWithComponents(components);
		}, "Find all entities with components", MetaFunc::ExplicitParams<const std::vector<ComponentFilter>&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const glm::vec2& screenPosition, float distanceFromCamera)
		{
			World* world = TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			return world->GetRenderer().ScreenToWorld(screenPosition, distanceFromCamera);
		}, "ScreenToWorld", MetaFunc::ExplicitParams<const glm::vec2&, float>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

	return type;
}
