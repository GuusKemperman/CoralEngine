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
	try
	{
		Device::sIsHeadless = argc >= 2
			&& strcmp(argv[1], "run_tests") == 0;

		FileIO::StartUp(argc, argv, gameDir);
		Logger::StartUp();

		if (!Device::IsHeadless())
		{
			Device::StartUp();
		}

		Input::StartUp();
#ifdef PLATFORM_WINDOWS
		if (!Device::IsHeadless())
		{
			Device::Get().CreateImguiContext();
		}
#endif
		MetaManager::StartUp();
		AssetManager::StartUp();
		VirtualMachine::StartUp();

#ifdef EDITOR
		Editor::StartUp();
#endif // !EDITOR

		UnitTestManager::StartUp();

		if (Device::sIsHeadless)
		{
			uint32 numFailed = 0;
			for (UnitTest& test : UnitTestManager::Get().GetAllTests())
			{
				test();
				if (test.mResult != UnitTest::Success)
				{
					numFailed++;
				}
			}

			// We only exit if numFailed != 0,
			// since maybe theres a crash in
			// the shutdown process and we want
			// to test that as well
			if (numFailed != 0)
			{
				// A lot of exits lead to exit(1).
				// by doing + 1 we can distinguish
				// from those errors
				exit(numFailed + 1);
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		exit(1);
	}
	catch (...)
	{
		std::cerr << "Unknown exception" << std::endl;
		exit(1);
	}
}

Engine::EngineClass::~EngineClass()
{
	//try
	//{
	//	UnitTestManager::ShutDown();

	//#ifdef EDITOR
	//	Editor::ShutDown();
	//#endif  // EDITOR

	//	VirtualMachine::ShutDown();
	//	AssetManager::ShutDown();
	//	MetaManager::ShutDown();
	//	Input::ShutDown();

	//	if (!Device::IsHeadless())
	//	{
	//		Device::ShutDown();
	//	}

	//	Logger::ShutDown();
	//	FileIO::ShutDown();
	//}
	//catch(const std::runtime_error& e)
	//{
	//	std::cerr << e.what() << std::endl;
	//	exit(1);
	//}
	//catch(...)
	//{
	//	std::cerr << "Unknown exception" << std::endl;
	//	exit(1);
	//}
}

void Engine::EngineClass::Run()
{
	if (Device::IsHeadless())
	{
		return;
	}

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

