#pragma once
#include "Assets/Asset.h"
#include "Components/ComponentFilter.h"
#include "Core/AssetHandle.h"

namespace CE
{
	class Texture;

	class Upgrade :
		public Asset
	{
	public:
		Upgrade(std::string_view name);
		Upgrade(AssetLoadInfo& loadInfo);

	private:
		ComponentFilter mUpgradeScript{};
		std::vector<ComponentFilter> mRequiredUpgrades{};
		bool mAllRequiredUpgradesNeeded{};
		AssetHandle<Texture> mIconTexture{};

		void OnSave(AssetSaveInfo& saveInfo) const override;

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(Upgrade);

	};
}
