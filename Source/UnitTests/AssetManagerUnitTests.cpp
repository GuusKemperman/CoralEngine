#include "Precomp.h"

#include "Core/UnitTests.h"
#include "Core/AssetManager.h"
#include "Core/ThreadPool.h"

using namespace CE;

UNIT_TEST(AssetManagerUnitTests, MultiThreadedAssetLoadingUnloading)
{
	static constexpr int amountOfLoadsToComplete = 2 << 11;

	const auto loadAssets = []
		{
			for (int i = 0; i < amountOfLoadsToComplete; i++)
			{
				for (AssetHandle asset : AssetManager::Get().GetAllAssets())
				{
					TEST_ASSERT(asset.Get() != nullptr);
				}
			}

			return UnitTest::Success;
		};

	const auto unloadAssets = []
		{
			for (int i = 0; i < amountOfLoadsToComplete; i++)
			{
				AssetManager::Get().UnloadAllUnusedAssets(false);
			}
		};

	std::vector<std::future<UnitTest::Result>> loadResults{};
	std::vector<std::future<void>> unloadResults{};

	for (size_t i = 0; i < ThreadPool::Get().NumberOfThreads(); i += 2)
	{
		loadResults.emplace_back(ThreadPool::Get().Enqueue(loadAssets));
		unloadResults.emplace_back(ThreadPool::Get().Enqueue(unloadAssets));
	}

	for (auto& result : loadResults)
	{
		TEST_ASSERT(result.get());
	}

	for (const auto& unloadResult : unloadResults)
	{
		unloadResult.wait();
	}

	return UnitTest::Success;
}

