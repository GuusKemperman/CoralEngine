#include "Precomp.h"
#include "Core/Engine.h"

#include <chrono>

#include "Core/FileIO.h"
#include "Core/Device.h"
#include "Core/AssetManager.h"
#include "Core/InputManager.h"
#include "Core/Editor.h"
#include "Core/VirtualMachine.h"
#include "Meta/MetaManager.h"
#include "Core/UnitTests.h"
#include "Assets/Level.h"
#include "World/World.h"
#include "World/WorldRenderer.h"
#include "Utilities/Benchmark.h"
#include "World/Registry.h"

Engine::EngineClass::EngineClass(int argc, char* argv[])
{
	FileIO::StartUp(argc == 0 ? std::string_view{} : argv[0]);
	Logger::StartUp();
	Device::StartUp();
	MetaManager::StartUp();
	AssetManager::StartUp();
	VirtualMachine::StartUp();

#ifdef EDITOR
	Editor::StartUp();
	UnitTestManager::StartUp();
#endif // !EDITOR
}

Engine::EngineClass::~EngineClass()
{
#ifdef EDITOR
	UnitTestManager::ShutDown();
	Editor::ShutDown();
#endif  // EDITOR

	VirtualMachine::ShutDown();
	AssetManager::ShutDown();
	MetaManager::ShutDown();
	Device::ShutDown();
	Logger::ShutDown();
	FileIO::ShutDown();
}

void Engine::EngineClass::Run()
{
#ifndef EDITOR
	// TODO level name is hardcoded
	std::shared_ptr<const Level> level = AssetManager::Get().TryGetAsset<Level>("DemoLevel");

	if (level == nullptr)
	{
		LOG(LogCore, Error, "Could not find starter level");
		return;
	}

	World world = level->CreateWorld(true);

	const MetaType* spawnerType = MetaManager::Get().TryGetType("DemoEnemySpawnerComponent");
	ASSERT(spawnerType != nullptr);

	const MetaField* desiredNumToSpawnField = spawnerType->TryGetField("DesiredNumOfEnemies");
	ASSERT(desiredNumToSpawnField != nullptr);

	entt::entity spawnerEntity = world.GetRegistry().FindEntityWithComponent(spawnerType);
	ASSERT(spawnerEntity != entt::null);
	MetaAny spawner = world.GetRegistry().Get(spawnerType->GetTypeId(), spawnerEntity);
	*desiredNumToSpawnField->MakeRef(spawner).As<int32>() = 1'000;
#endif // EDITOR

	Device& renderer = Device::Get();

	float timeElapsedSinceLastGarbageCollect{};
	static constexpr float garbageCollectInterval = 5.0f;

	[[maybe_unused]] float deltaTime;
	std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point t2{};

	while (!renderer.ShouldClose())
	{
		t2 = std::chrono::high_resolution_clock::now();
		deltaTime = (std::chrono::duration_cast<std::chrono::duration<float>>(t2 - t1)).count();
		t1 = t2;

		InputManager::NewFrame();
		renderer.NewFrame();

#ifdef EDITOR
		Editor::Get().Tick(deltaTime);
#else
		world.Tick(deltaTime);
		world.GetRenderer().Render();
#endif  // EDITOR

		renderer.Render();

		timeElapsedSinceLastGarbageCollect += deltaTime;

		if (timeElapsedSinceLastGarbageCollect > garbageCollectInterval)
		{
			AssetManager::Get().UnloadAllUnusedAssets();
			timeElapsedSinceLastGarbageCollect = 0.0f;
		}
	}
}

int main(int argc, char* args[])
{
	Engine::EngineClass engine{ argc, args };
	engine.Run();
	return 0;
}