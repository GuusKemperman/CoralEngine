#pragma once
#include "Assets/Core/AssetHandle.h"
#include "Systems/System.h"

namespace Game
{
	class Upgrade;

	class UpgradeSystem final :
		public CE::System
	{
	public:
		void Update(CE::World& world, float dt) override;
		void OnLevelUp(CE::World& world);

	private:
		std::vector<CE::AssetHandle<Upgrade>> mAvailableUpgrades{};
		static constexpr int sNumberOfUpgradesToDisplay = 4;
		std::vector< CE::AssetHandle<Upgrade>> mChosenUpgradesToDisplayThisLevel{};

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(UpgradeSystem);
	};
}
