#include "Precomp.h"
#include "Assets/Upgrade.h"

#include "Assets/Core/AssetLoadInfo.h"
#include "Assets/Core/AssetSaveInfo.h"
#include "Meta/Fwd/MetaTypeFwd.h"
#include "Utilities/Reflect/ReflectAssetType.h"
#include "Assets/Texture.h"
#include "Assets/Script.h"

using namespace CE;

Game::Upgrade::Upgrade(std::string_view name) :
	Asset(name, MakeTypeId<Upgrade>())
{
}

Game::Upgrade::Upgrade(AssetLoadInfo& loadInfo) :
	Asset(loadInfo)
{
	BinaryGSONObject obj{};
	const bool success = obj.LoadFromBinary(loadInfo.GetStream());

	if (!success)
	{
		LOG(LogAssets, Error, "Could not load upgrade {}, GSON parsing failed.", GetName());
		return;
	}

	const BinaryGSONMember* serializedUpgradeScript = obj.TryGetGSONMember("mUpgradeScript");
	const BinaryGSONMember* serializedRequiredUpgrades = obj.TryGetGSONMember("mRequiredUpgrades");
	const BinaryGSONMember* serializedAllRequiredUpgradesNeeded = obj.TryGetGSONMember("mAllRequiredUpgradesNeeded");
	const BinaryGSONMember* serializedIconTexture = obj.TryGetGSONMember("mIconTexture");

	if (serializedUpgradeScript == nullptr
		|| serializedIconTexture == nullptr
		|| serializedRequiredUpgrades == nullptr
		|| serializedAllRequiredUpgradesNeeded == nullptr)
	{
		LOG(LogAssets, Error, "Could not load upgrade {}, as there were missing values.", GetName());
		return;
	}

	*serializedUpgradeScript >> mUpgradeScript;
	*serializedRequiredUpgrades >> mRequiredUpgrades;
	*serializedAllRequiredUpgradesNeeded >> mAllRequiredUpgradesNeeded;
	*serializedIconTexture >> mIconTexture;
}

void Game::Upgrade::OnSave(AssetSaveInfo& saveInfo) const
{
	BinaryGSONObject obj{};

	obj.AddGSONMember("mUpgradeScript") << mUpgradeScript;
	obj.AddGSONMember("mRequiredUpgrades") << mRequiredUpgrades;
	obj.AddGSONMember("mAllRequiredUpgradesNeeded") << mAllRequiredUpgradesNeeded;
	obj.AddGSONMember("mIconTexture") << mIconTexture;

	obj.SaveToBinary(saveInfo.GetStream());
}

CE::MetaType Game::Upgrade::Reflect()
{
	MetaType type = MetaType{ MetaType::T<Upgrade>{}, "Upgrade", MetaType::Base<Asset>{}, MetaType::Ctor<AssetLoadInfo&>{}, MetaType::Ctor<std::string_view>{} };
	type.GetProperties().Add(Props::sIsScriptableTag);

	type.AddField(&Upgrade::mUpgradeScript, "mUpgradeScript").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&Upgrade::mRequiredUpgrades, "mRequiredUpgrades").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&Upgrade::mAllRequiredUpgradesNeeded, "mAllRequiredUpgradesNeeded").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&Upgrade::mIconTexture, "mIconTexture").GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Upgrade>& upgrade) -> ComponentFilter
		{
			if (upgrade == nullptr)
			{
				return nullptr;
			}

			return upgrade->mUpgradeScript;
		},
		"GetUpgradeScript", MetaFunc::ExplicitParams<const AssetHandle<Upgrade>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Upgrade>& upgrade) -> std::vector<ComponentFilter>
		{
			if (upgrade == nullptr)
			{
				return {};
			}

			return upgrade->mRequiredUpgrades;
		},
		"GetRequiredUpgrades", MetaFunc::ExplicitParams<const AssetHandle<Upgrade>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Upgrade>& upgrade) -> bool
		{
			if (upgrade == nullptr)
			{
				return false;
			}

			return upgrade->mAllRequiredUpgradesNeeded;
		},
		"GetAllRequiredUpgradesNeeded", MetaFunc::ExplicitParams<const AssetHandle<Upgrade>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Upgrade>& upgrade) -> AssetHandle<Texture>
		{
			if (upgrade == nullptr)
			{
				return nullptr;
			}

			return upgrade->mIconTexture;
		},
		"GetIconTexture", MetaFunc::ExplicitParams<const AssetHandle<Upgrade>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	ReflectAssetType<Upgrade>(type);
	return type;
}
