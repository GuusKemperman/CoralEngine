#pragma once
#include "Assets/Core/AssetHandle.h"
#include "Meta/Fwd/MetaReflectFwd.h"

namespace CE
{
	class World;
}

namespace Game
{
	class Upgrade;

	class UpgradeFunctionality
	{
	public:
		static std::vector<CE::AssetHandle<Upgrade>> GetAvailableUpgrades(CE::World& world, int numberOfOptions);
		static void InitializeUpgradeOptions(CE::World& world, std::vector<entt::entity>& options, std::vector<CE::AssetHandle<Upgrade>>& upgradesToExclude);

	private:
		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(UpgradeFunctionality);
	};
}

