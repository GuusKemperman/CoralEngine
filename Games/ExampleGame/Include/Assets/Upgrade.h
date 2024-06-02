#pragma once
#include "Assets/Asset.h"
#include "Assets/Core/AssetHandle.h"
#include "Components/ComponentFilter.h"

namespace CE
{
	class Texture;
}

namespace Game
{
	class Upgrade :
		public CE::Asset
	{
	public:
		Upgrade(std::string_view name);
		Upgrade(CE::AssetLoadInfo& loadInfo);

		CE::ComponentFilter mUpgradeComponent{};
		std::vector<CE::ComponentFilter> mRequiredUpgrades{};
		bool mAllRequiredUpgradesNeeded{};
		CE::AssetHandle<CE::Texture> mIconTexture{};

	private:
		void OnSave(CE::AssetSaveInfo& saveInfo) const override;

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(Upgrade);

	};
}
