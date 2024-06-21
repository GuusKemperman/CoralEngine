#include "Precomp.h"
#include "Utilities/UpgradeFunctionality.h"

#include "Assets/Upgrade.h"
#include "Components/PlayerComponent.h"
#include "Components/UpgradeStoringComponent.h"
#include "Components/UI/UISpriteComponent.h"
#include "Core/AssetManager.h"
#include "Utilities/Random.h"
#include "World/Registry.h"
#include "World/World.h"

std::vector<CE::AssetHandle<Game::Upgrade>> Game::UpgradeFunctionality::GetAvailableUpgrades(CE::World& world,
	int numberOfOptions)
{
	auto& registry = world.GetRegistry();
	const auto playerView = registry.View<CE::PlayerComponent>();
	const entt::entity playerEntity = playerView.front();

	// Iterate over all the upgrades to check what upgrades the player has and update the available upgrades.
	std::vector<CE::AssetHandle<Upgrade>> availableUpgrades{};
	std::vector<CE::AssetHandle<Upgrade>> chosenUpgradesToDisplayThisLevel{};
	for (CE::WeakAssetHandle<Upgrade> upgrade : CE::AssetManager::Get().GetAllAssets<Upgrade>())
	{
		const CE::AssetHandle<Upgrade> loadedUpgrade{ upgrade };

		if (loadedUpgrade->mUpgradeComponent == nullptr ||
			registry.HasComponent(loadedUpgrade->mUpgradeComponent.Get()->GetTypeId(), playerEntity))
		{
			continue;
		}
		if (loadedUpgrade->mRequiredUpgrades.empty())
		{
			availableUpgrades.push_back(loadedUpgrade);
			continue;
		}
		bool allRequiredUpgradesUnlocked = true;
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
					availableUpgrades.push_back(loadedUpgrade);
					break;
				}
			}
			else
			{
				allRequiredUpgradesUnlocked = false;
				if (loadedUpgrade->mAllRequiredUpgradesNeeded)
				{
					break;
				}
			}
		}
		if (loadedUpgrade->mAllRequiredUpgradesNeeded && allRequiredUpgradesUnlocked)
		{
			availableUpgrades.push_back(loadedUpgrade);
		}
	}

	if (availableUpgrades.empty())
	{
		return {};
	}

	// Randomly choose upgrades to display.
	for (int i = 0; i < numberOfOptions && !availableUpgrades.empty(); i++)
	{
		const int randomNumber = CE::Random::Range(0, static_cast<int>(availableUpgrades.size()));
		chosenUpgradesToDisplayThisLevel.push_back(availableUpgrades[randomNumber]);
		availableUpgrades[randomNumber] = std::move(availableUpgrades.back());
		availableUpgrades.pop_back();
	}
	return chosenUpgradesToDisplayThisLevel;
}

void Game::UpgradeFunctionality::InitializeUpgradeOptions(CE::World& world, std::vector<entt::entity>& options, std::vector<CE::AssetHandle<Upgrade>>& upgradesToExclude)
{
	auto chosenUpgradesToDisplayThisLevel = GetAvailableUpgrades(world, static_cast<int>(options.size()));

	chosenUpgradesToDisplayThisLevel.erase(std::remove_if(chosenUpgradesToDisplayThisLevel.begin(), chosenUpgradesToDisplayThisLevel.end(),
		[&upgradesToExclude](int value) {
			return std::find(upgradesToExclude.begin(), upgradesToExclude.end(), value) != upgradesToExclude.end();
		}),
		chosenUpgradesToDisplayThisLevel.end());

	auto& registry = world.GetRegistry();
	const size_t numberOfOptions = options.size();
	const size_t numberOfAvailableChosenUpgrades = chosenUpgradesToDisplayThisLevel.size();
	for (size_t i = 0; i < numberOfAvailableChosenUpgrades && i < numberOfOptions; i++)
	{
		auto sprite = registry.TryGet<CE::UISpriteComponent>(options[i]);
		if (sprite == nullptr)
		{
			LOG(LogUpgradeFunctionality, Warning, "Entity {} does not have a UISpriteComponent attached.", entt::to_integral(options[i]));
			continue;
		}
		auto upgrade = registry.TryGet<UpgradeStoringComponent>(options[i]);
		if (upgrade == nullptr)
		{
			LOG(LogUpgradeFunctionality, Warning, "Entity {} does not have a UpgradeStoringComponent attached.", entt::to_integral(options[i]));
			continue;
		}
		sprite->mTexture = chosenUpgradesToDisplayThisLevel[i].Get()->mIconTexture;
		upgrade->mUpgrade = chosenUpgradesToDisplayThisLevel[i];
	}
}

CE::MetaType Game::UpgradeFunctionality::Reflect()
{
	auto metaType = CE::MetaType{ CE::MetaType::T<UpgradeFunctionality>{}, "UpgradeFunctionality" };
	metaType.GetProperties().Add(CE::Props::sIsScriptableTag);

	metaType.AddFunc([](std::vector<entt::entity> options, std::vector<CE::AssetHandle<Upgrade>> upgradesToExclude)
		{
			CE::World* world = CE::World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			InitializeUpgradeOptions(*world, options, upgradesToExclude);

		}, "InitializeUpgradeOptions", CE::MetaFunc::ExplicitParams<
		std::vector<entt::entity>, std::vector<CE::AssetHandle<Upgrade>>>{}, "Options", "Upgrades To Exclude").GetProperties().Add(CE::Props::sIsScriptableTag).Set(CE::Props::sIsScriptPure, false);

		return metaType;
}
