#include "Precomp.h"
#include "Systems/UpgradeSystem.h"

#include "Assets/Upgrade.h"
#include "Components/PlayerComponent.h"
#include "Components/UI/UISpriteComponent.h"
#include "Core/AssetManager.h"
#include "Meta/Fwd/MetaTypeFwd.h"
#include "Utilities/Random.h"
#include "World/Registry.h"
#include "World/World.h"

void Game::UpgradeSystem::Update(CE::World&, float)
{
}

std::vector<CE::WeakAssetHandle<Game::Upgrade>> Game::UpgradeSystem::OnLevelUp(CE::World& world, int numberOfOptions)
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
		if (loadedUpgrade->mRequiredUpgrades.empty())
		{
			availableUpgrades.push_back(upgrade); // should I be storing weak assets instead?
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
				break;
			}
		}
		if (loadedUpgrade->mAllRequiredUpgradesNeeded && allRequireUpgradesUnlocked)
		{
			availableUpgrades.push_back(upgrade);
		}

		if (availableUpgrades.empty())
		{
			break;
		}

		// Randomly choose upgrades to display.
		for (int i = 0; i < numberOfOptions; i++)
		{
			const int randomNumber = CE::Random::Range(0, static_cast<int>(availableUpgrades.size()));
			chosenUpgradesToDisplayThisLevel.push_back(availableUpgrades[randomNumber]);
			availableUpgrades.erase(availableUpgrades.begin() + randomNumber);
		}
	}
	return chosenUpgradesToDisplayThisLevel;
}

void Game::UpgradeSystem::InitializeUpgradeOptions(CE::World& world, std::vector<entt::entity>& options)
{
	const auto chosenUpgradesToDisplayThisLevel = OnLevelUp(world, static_cast<int>(options.size()));

	if (chosenUpgradesToDisplayThisLevel.empty())
	{
		return;
	}

	auto& registry = world.GetRegistry();
	int i{};
	for (auto entity : options)
	{
		auto sprite = registry.TryGet<CE::UISpriteComponent>(entity);
		if (sprite == nullptr)
		{
			LOG(LogUpgradeSystem, Warning, "Entity {} does not have a UISpriteComponent attached.", entt::to_integral(entity));
			continue;
		}
		const CE::AssetHandle<Upgrade> loadedUpgrade{ chosenUpgradesToDisplayThisLevel[i++] };
		sprite->mTexture = loadedUpgrade.Get()->mIconTexture;
	}
}

CE::MetaType Game::UpgradeSystem::Reflect()
{
	auto metaType = CE::MetaType{ CE::MetaType::T<UpgradeSystem>{}, "UpgradeSystem", CE::MetaType::Base<System>{} };
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
