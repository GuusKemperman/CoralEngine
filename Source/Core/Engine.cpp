#include "Precomp.h"
#include "Core/Engine.h"

#include <chrono>

#include "Core/FileIO.h"
#include "Core/Device.h"
#include "Core/AssetManager.h"
#include "Core/Input.h"
#include "Core/Editor.h"
#include "Core/VirtualMachine.h"
#include "Meta/MetaManager.h"
#include "Core/UnitTests.h"
#include "Assets/Level.h"
#include "World/World.h"
#include "World/WorldRenderer.h"
#include "Utilities/Benchmark.h"
#include "World/Registry.h"

Engine::EngineClass::EngineClass(int argc, char* argv[], std::string_view gameDir)
{
	FileIO::StartUp(argc, argv, gameDir);
	Logger::StartUp();
	Device::StartUp();
	Input::StartUp();
#ifdef PLATFORM_WINDOWS
	Device::Get().CreateImguiContext();
#endif
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
	Input::ShutDown();
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
#endif // EDITOR

	Input& input = Input::Get();
	Device& device = Device::Get();

	float timeElapsedSinceLastGarbageCollect{};
	static constexpr float garbageCollectInterval = 5.0f;

	[[maybe_unused]] float deltaTime;
	std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point t2{};

	while (!device.ShouldClose())
	{
		t2 = std::chrono::high_resolution_clock::now();
		deltaTime = (std::chrono::duration_cast<std::chrono::duration<float>>(t2 - t1)).count();
		t1 = t2;

		input.NewFrame();

#ifdef EDITOR
		Editor::Get().Tick(deltaTime);
#else
		device.NewFrame();
		world.Tick(deltaTime);
		world.GetRenderer().Render();
		device.EndFrame();
#endif  // EDITOR

		timeElapsedSinceLastGarbageCollect += deltaTime;

		if (timeElapsedSinceLastGarbageCollect > garbageCollectInterval)
		{
			AssetManager::Get().UnloadAllUnusedAssets();
			timeElapsedSinceLastGarbageCollect = 0.0f;
		}
	}
}

