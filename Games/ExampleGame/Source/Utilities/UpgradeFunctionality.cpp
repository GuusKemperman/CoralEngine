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

std::vector<CE::WeakAssetHandle<Game::Upgrade>> Game::UpgradeFunctionality::GetAvailableUpgrades(CE::World& world,
	int numberOfOptions)
{
	auto& registry = world.GetRegistry();
	const auto playerView = registry.View<CE::PlayerComponent>();
	const entt::entity playerEntity = playerView.front();

	// Iterate over all the upgrades to check what upgrades the player has and update the available upgrades.
	std::vector<CE::WeakAssetHandle<Upgrade>> availableUpgrades{};
	std::vector<CE::WeakAssetHandle<Upgrade>> chosenUpgradesToDisplayThisLevel{};
	for (CE::WeakAssetHandle<Upgrade> upgrade : CE::AssetManager::Get().GetAllAssets<Upgrade>())
	{
		const CE::AssetHandle<Upgrade> loadedUpgrade{ upgrade };

		if (loadedUpgrade->mUpgradeScript == nullptr)
		{
			continue;
		}
		if (loadedUpgrade->mRequiredUpgrades.empty() &&
			!registry.HasComponent(loadedUpgrade->mUpgradeScript.Get()->GetTypeId(), playerEntity))
		{
			availableUpgrades.push_back(upgrade);
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
					availableUpgrades.push_back(upgrade);
					break;
				}
			}
			else
			{
				allRequireUpgradesUnlocked = false;
				if (loadedUpgrade->mAllRequiredUpgradesNeeded)
				{
					break;
				}
			}
		}
		if (loadedUpgrade->mAllRequiredUpgradesNeeded && allRequireUpgradesUnlocked)
		{
			availableUpgrades.push_back(upgrade);
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
		availableUpgrades.erase(availableUpgrades.begin() + randomNumber);
	}
	return chosenUpgradesToDisplayThisLevel;
}

void Game::UpgradeFunctionality::InitializeUpgradeOptions(CE::World& world, std::vector<entt::entity>& options)
{
	const auto chosenUpgradesToDisplayThisLevel = GetAvailableUpgrades(world, static_cast<int>(options.size()));

	auto& registry = world.GetRegistry();
	const size_t numberOfOptions = options.size();
	const size_t numberOfAvailableChosenUpgrades = chosenUpgradesToDisplayThisLevel.size();
	for (size_t i = 0; i < numberOfAvailableChosenUpgrades && i < numberOfOptions; i++)
	{
		auto sprite = registry.TryGet<CE::UISpriteComponent>(options[i]);
		if (sprite == nullptr)
		{
			LOG(LogUpgradeSystem, Warning, "Entity {} does not have a UISpriteComponent attached.", entt::to_integral(options[i]));
			continue;
		}
		auto upgrade = registry.TryGet<UpgradeStoringComponent>(options[i]);
		if (upgrade == nullptr)
		{
			LOG(LogUpgradeSystem, Warning, "Entity {} does not have a UpgradeStoringComponent attached.", entt::to_integral(options[i]));
			continue;
		}
		const CE::AssetHandle<Upgrade> loadedUpgrade{ chosenUpgradesToDisplayThisLevel[i] };
		sprite->mTexture = loadedUpgrade.Get()->mIconTexture;
		upgrade->mUpgrade = loadedUpgrade;
	}
}

CE::MetaType Game::UpgradeFunctionality::Reflect()
{
	auto metaType = CE::MetaType{ CE::MetaType::T<UpgradeFunctionality>{}, "UpgradeFunctionality" };
	metaType.GetProperties().Add(CE::Props::sIsScriptableTag);

	metaType.AddFunc([](std::vector<entt::entity> options)
		{
			CE::World* world = CE::World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			InitializeUpgradeOptions(*world, options);

		}, "InitializeUpgradeOptions", CE::MetaFunc::ExplicitParams<
		std::vector<entt::entity>>{}, "Options").GetProperties().Add(CE::Props::sIsScriptableTag).Set(CE::Props::sIsScriptPure, false);

	return metaType;
}
