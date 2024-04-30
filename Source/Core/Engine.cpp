#include "Precomp.h"
#include "Core/Engine.h"

#include <chrono>

#include "Core/FileIO.h"
#include "Core/Device.h"
#include "Core/AssetManager.h"
#include "Core/Input.h"
#include "Core/Editor.h"
#include "Core/VirtualMachine.h"
#include "Core/JobManager.h"
#include "Meta/MetaManager.h"
#include "Core/UnitTests.h"
#include "Assets/Level.h"
#include "World/World.h"
#include "Utilities/Benchmark.h"
#include "World/Registry.h"
#include "Rendering/Renderer.h"

CE::Engine::Engine(int argc, char* argv[], std::string_view gameDir)
{
	Device::sIsHeadless = argc >= 2
		&& strcmp(argv[1], "run_tests") == 0;

	FileIO::StartUp(argc, argv, gameDir);
	Logger::StartUp();

	std::thread deviceAgnosticSystems
	{
		[&]
		{
			JobManager::StartUp();
			MetaManager::StartUp();
			AssetManager::StartUp();
			VirtualMachine::StartUp();
		}
	};

	if (!Device::IsHeadless())
	{
		Device::StartUp();
		Renderer::StartUp();
	}

	Input::StartUp();
#ifdef EDITOR
	if (!Device::IsHeadless())
	{
		Device::Get().CreateImguiContext();
	}
#endif // EDITOR

	deviceAgnosticSystems.join();

#ifdef EDITOR
	Editor::StartUp();
#endif // EDITOR

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

CE::Engine::~Engine()
{
	UnitTestManager::ShutDown();

#ifdef EDITOR
	Editor::ShutDown();
#endif  // EDITOR

	VirtualMachine::ShutDown();
	AssetManager::ShutDown();
	MetaManager::ShutDown();
	Input::ShutDown();

	if (!Device::IsHeadless())
	{
		Renderer::ShutDown();
		Device::ShutDown();
	}

	Logger::ShutDown();
	FileIO::ShutDown();
	JobManager::ShutDown();
}

void CE::Engine::Run([[maybe_unused]] Name starterLevel)
{
	if (Device::IsHeadless())
	{
		return;
	}

#ifndef EDITOR
	AssetHandle<Level> level = AssetManager::Get().TryGetAsset<Level>(starterLevel);

	if (level == nullptr)
	{
		LOG(LogCore, Error, "Could not find starter level");
		return;
	}

	World world = level->CreateWorld(true);
#endif // EDITOR

	Input& input = Input::Get();
	Device& device = Device::Get();

#ifdef EDITOR
	Editor& editor = Editor::Get();
#endif // EDITOR

	float timeElapsedSinceLastGarbageCollect{};
	static constexpr float garbageCollectInterval = 15.0f;

	std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point t2{};

	while (!device.ShouldClose())
	{
		t2 = std::chrono::high_resolution_clock::now();
		float deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(t2 - t1).count();

		// Check if we hit a breakpoint or something else
		// that interrupted the program for an extended duration
		if (deltaTime > 5.0f)
		{
			deltaTime = 1.0f / 60.0f;
		}

		t1 = t2;

		input.NewFrame();
		device.NewFrame();

		if (device.GetDisplaySize().x <= 0
			|| device.GetDisplaySize().y <= 0)
		{
			continue;
		}

#ifdef EDITOR
		editor.Tick(deltaTime);
#else
		world.Tick(deltaTime);

		if (!Device::IsHeadless())
		{
			Renderer::Get().Render(world);
		}
#endif  // EDITOR

		device.EndFrame();

#ifdef EDITOR
		// Has to be done after EndFrame for some dx12 reasons,
		// as you are not allowed to free resources in between NewFrame and EndFrame.
		editor.FullFillRefreshRequests();
#endif // EDITOR

		timeElapsedSinceLastGarbageCollect += deltaTime;

		if (timeElapsedSinceLastGarbageCollect > garbageCollectInterval)
		{
			AssetManager::Get().UnloadAllUnusedAssets();
			timeElapsedSinceLastGarbageCollect = 0.0f;
		}
	}
}
