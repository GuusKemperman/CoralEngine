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
		static std::vector<CE::WeakAssetHandle<Upgrade>> OnLevelUp(CE::World& world, int numberOfOptions);
		static void InitializeUpgradeOptions(CE::World& world, std::vector<entt::entity>& options);

	private:
		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(UpgradeSystem);
	};
}
