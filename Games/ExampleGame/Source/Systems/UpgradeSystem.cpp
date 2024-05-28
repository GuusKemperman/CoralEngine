#include "Precomp.h"
#include "Systems/UpgradeSystem.h"

#include "Assets/Upgrade.h"
#include "Components/PlayerComponent.h"
#include "Core/AssetManager.h"
#include "Meta/Fwd/MetaTypeFwd.h"
#include "Utilities/Random.h"
#include "World/Registry.h"
#include "World/World.h"

void Game::UpgradeSystem::Update(CE::World&, float)
{
}

void Game::UpgradeSystem::OnLevelUp(CE::World& world)
{
	auto& registry = world.GetRegistry();
	const auto playerView = registry.View<CE::PlayerComponent>();
	const entt::entity playerEntity = playerView.front();

	// Iterate over all the upgrades to check what upgrades the player has and update the available upgrades.
	mAvailableUpgrades.clear();
	for (CE::WeakAssetHandle<Upgrade> upgrade : CE::AssetManager::Get().GetAllAssets<Upgrade>())
	{
		const CE::AssetHandle<Upgrade> loadedUpgrade{ upgrade };

		if (loadedUpgrade->mUpgradeScript == nullptr)
		{
			continue;
		}
		if (loadedUpgrade->mRequiredUpgrades.empty())
		{
			mAvailableUpgrades.push_back(loadedUpgrade); // should I be storing weak assets instead?
			continue;
		}
		bool allRequireUpgradesUnlocked = true;
		for (auto& requiredUpgrade : loadedUpgrade->mRequiredUpgrades)
		{
			if (requiredUpgrade == nullptr)
			{
				continue;
			}
			if (registry.HasComponent(requiredUpgrade.Get()->GetTypeId(), playerEntity))
			{
				if (loadedUpgrade->mAllRequiredUpgradesNeeded == false)
				{
					mAvailableUpgrades.push_back(loadedUpgrade);
					break;
				}
			}
			else
			{
				allRequireUpgradesUnlocked = false;
				break;
			}
		}
		if (loadedUpgrade->mAllRequiredUpgradesNeeded && allRequireUpgradesUnlocked)
		{
			mAvailableUpgrades.push_back(loadedUpgrade);
		}

		// Randomly choose upgrades to display.
		mChosenUpgradesToDisplayThisLevel.clear();
		for (int i = 0; i < sNumberOfUpgradesToDisplay; i++)
		{
			const int randomNumber = CE::Random::Range(0, static_cast<int>(mAvailableUpgrades.size()));
			mChosenUpgradesToDisplayThisLevel.push_back(mAvailableUpgrades[randomNumber]);
			mAvailableUpgrades.erase(mAvailableUpgrades.begin() + randomNumber);
		}
	}
}

CE::MetaType Game::UpgradeSystem::Reflect()
{
	return CE::MetaType{ CE::MetaType::T<UpgradeSystem>{}, "UpgradeSystem", CE::MetaType::Base<System>{} };
}