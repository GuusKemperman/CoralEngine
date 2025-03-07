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
#include "Core/Audio.h"
#include "Assets/Level.h"
#include "World/World.h"
#include "Utilities/Benchmark.h"
#include "World/Registry.h"
#include "Rendering/Renderer.h"

// For the UNIT_TEST_DECLARATION macro used in the Generated file
#include "Core/UnitTests.h"

// Forces initialization of otherwise unused static variables
#include "../Intermediate/Generated/Generated.h"

CE::Engine::Engine(int argc, char* argv[], std::string_view gameDir)
{
	Device::sIsHeadless = argc >= 2
		&& strcmp(argv[1], "run_tests") == 0;

	FileIO::StartUp(argc, argv, gameDir);
	Logger::StartUp();

	LOG(LogCore, Verbose, "Created logger");

	std::thread deviceAgnosticSystems
	{
		[&]
		{
			LOG(LogCore, Verbose, "Creating MetaManager");
			MetaManager::StartUp();
			LOG(LogCore, Verbose, "Creating AssetManager");
			AssetManager::StartUp();
			LOG(LogCore, Verbose, "Booting up virtual machine");
			VirtualMachine::StartUp();
		}
	};

	if (!Device::IsHeadless())
	{
		LOG(LogCore, Verbose, "Creating window & device");
		Device::StartUp();
		LOG(LogCore, Verbose, "Creating renderer");
		Renderer::StartUp();
	}

	LOG(LogCore, Verbose, "Creating Audio");
	Audio::StartUp();

	LOG(LogCore, Verbose, "Creating Input");
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

	LOG(LogCore, Verbose, "Creating UnitTestManager");
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

		const uint32 numOfErrorsLogged = Logger::Get().GetNumOfEntriesPerSeverity()[LogSeverity::Error];
		if (numOfErrorsLogged != 0)
		{
			LOG(LogUnitTest, Error, "There were {} unresolved errors logged to the consoler", numOfErrorsLogged);
			numFailed += numOfErrorsLogged;
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

	LOG(LogCore, Verbose, "Completed engine startup");
}

CE::Engine::~Engine()
{
	LOG(LogCore, Verbose, "Shutting down UnitTestManager");
	UnitTestManager::ShutDown();

#ifdef EDITOR
	LOG(LogCore, Verbose, "Shutting down Editor");
	Editor::ShutDown();
#endif  // EDITOR

	LOG(LogCore, Verbose, "Shutting down VirtualMachine");
	VirtualMachine::ShutDown();
	LOG(LogCore, Verbose, "Shutting down AssetManager");
	AssetManager::ShutDown();
	LOG(LogCore, Verbose, "Shutting down MetaManager");
	MetaManager::ShutDown();
	LOG(LogCore, Verbose, "Shutting down Audio");
	Audio::ShutDown();
	LOG(LogCore, Verbose, "Shutting down Input");
	Input::ShutDown();

	if (!Device::IsHeadless())
	{
		LOG(LogCore, Verbose, "Shutting down Renderer");
		Renderer::ShutDown();
		LOG(LogCore, Verbose, "Shutting down Device");
		Device::ShutDown();
	}

	LOG(LogCore, Verbose, "Shutting down Logger");
	Logger::ShutDown();
	FileIO::ShutDown();
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

	LOG(LogCore, Verbose, "Loading initial level - {}", starterLevel.StringView());
	World world = level->CreateWorld(true);
#endif // EDITOR

	Input& input = Input::Get();
	Device& device = Device::Get();

#ifdef EDITOR
	Editor& editor = Editor::Get();
#endif // EDITOR

	float timeElapsedSinceLastGarbageCollect{};
	static constexpr float garbageCollectInterval = 120.0f;

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

		device.NewFrame();
		input.NewFrame();

		if (device.GetDisplaySize().x <= 0
			|| device.GetDisplaySize().y <= 0)
		{
			continue;
		}

#ifdef EDITOR
		editor.Tick(deltaTime);
#else
		world.Tick(deltaTime);

		if (world.HasRequestedEndPlay())
		{
			break;
		}

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
