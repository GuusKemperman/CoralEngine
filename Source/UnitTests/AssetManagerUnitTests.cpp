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
		};

	const auto unloadAssets = []
		{
			for (int i = 0; i < amountOfLoadsToComplete; i++)
			{
				AssetManager::Get().UnloadAllUnusedAssets(false);
			}
		};

	std::vector<std::future<void>> loadResults{};
	std::vector<std::future<void>> unloadResults{};

	loadResults.emplace_back(ThreadPool::Get().Enqueue(loadAssets));
	unloadResults.emplace_back(ThreadPool::Get().Enqueue(unloadAssets));

	for (auto& result : loadResults)
	{
		result.wait();
	}

	for (const auto& unloadResult : unloadResults)
	{
		unloadResult.wait();
	}
}

