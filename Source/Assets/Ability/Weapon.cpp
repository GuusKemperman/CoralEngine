#include "Precomp.h"
#include "Assets/Ability/Weapon.h"

#include "Assets/Core/AssetLoadInfo.h"
#include "Assets/Core/AssetSaveInfo.h"
#include "Utilities/Reflect/ReflectAssetType.h"
#include "Assets/Texture.h"
#include "Assets/Script.h"

CE::Weapon::Weapon(std::string_view name) :
	Ability(name, MakeTypeId<Weapon>())
{
}

CE::Weapon::Weapon(AssetLoadInfo& loadInfo) :
	Ability(loadInfo)
{
	BinaryGSONObject obj{};
	const bool success = obj.LoadFromBinary(loadInfo.GetStream());

	if (!success)
	{
		LOG(LogAssets, Error, "Could not load weapon {}, GSON parsing failed.", GetName());
		return;
	}

	const BinaryGSONMember* serializedTimeBetweenShots = obj.TryGetGSONMember("TimeBetweenShots");
	const BinaryGSONMember* serializedFireSpeed = obj.TryGetGSONMember("FireSpeed");
	const BinaryGSONMember* serializedReloadSpeed = obj.TryGetGSONMember("ReloadSpeed");
	const BinaryGSONMember* serializedProjectileCount = obj.TryGetGSONMember("ProjectileCount");
	const BinaryGSONMember* serializedSpread = obj.TryGetGSONMember("Spread");

	const BinaryGSONMember* serializedEffects = obj.TryGetGSONMember("Effects");
	const BinaryGSONMember* serializedProjectileSize = obj.TryGetGSONMember("ProjectileSize");
	const BinaryGSONMember* serializedProjectileSpeed = obj.TryGetGSONMember("ProjectileSpeed");
	const BinaryGSONMember* serializedProjectileRange = obj.TryGetGSONMember("ProjectileRange");
	const BinaryGSONMember* serializedKnockback = obj.TryGetGSONMember("Knockback");
	const BinaryGSONMember* serializedPiercing = obj.TryGetGSONMember("Piercing");

	if (serializedTimeBetweenShots == nullptr
		|| serializedFireSpeed == nullptr
		|| serializedReloadSpeed == nullptr
		|| serializedProjectileCount == nullptr
		|| serializedSpread == nullptr 
		|| serializedEffects == nullptr
		|| serializedProjectileSize == nullptr
		|| serializedProjectileSpeed == nullptr
		|| serializedProjectileRange == nullptr
		|| serializedKnockback == nullptr
		|| serializedPiercing == nullptr)
	{
		LOG(LogAssets, Error, "Could not load weapon {}, as there were missing values.", GetName());
		return;
	}

	*serializedTimeBetweenShots >> mOnAbilityActivateScript;
	*serializedFireSpeed >> mIconTexture;
	*serializedReloadSpeed >> mDescription;
	*serializedProjectileCount >> mGlobalCooldown;
	*serializedSpread >> mRequirementType;

	*serializedEffects >> mEffects;
	*serializedProjectileSize >> mProjectileSize;
	*serializedProjectileSpeed >> mProjectileSpeed;
	*serializedProjectileRange >> mProjectileRange;
	*serializedKnockback >> mKnockback;
	*serializedPiercing >> mPiercing;
}

void CE::Weapon::OnSave(AssetSaveInfo& saveInfo) const
{
	Ability::OnSave(saveInfo);

	BinaryGSONObject obj{};

	obj.AddGSONMember("TimeBetweenShots") << mTimeBetweenShots;
	obj.AddGSONMember("FireSpeed") << mFireSpeed;
	obj.AddGSONMember("ReloadSpeed") << mReloadSpeed;
	obj.AddGSONMember("ProjectileCount") << mProjectileCount;
	obj.AddGSONMember("Spread") << mSpread;

	obj.AddGSONMember("Effects") << mEffects;
	obj.AddGSONMember("ProjectileSize") << mProjectileSize;
	obj.AddGSONMember("ProjectileSpeed") << mProjectileSpeed;
	obj.AddGSONMember("ProjectileRange") << mProjectileRange;
	obj.AddGSONMember("Knockback") << mKnockback;
	obj.AddGSONMember("Piercing") << mPiercing;

	obj.SaveToBinary(saveInfo.GetStream());
}

CE::MetaType CE::Weapon::Reflect()
{
	MetaType type = MetaType{ MetaType::T<Weapon>{}, "Weapon", MetaType::Base<Ability>{}, MetaType::Ctor<AssetLoadInfo&>{}, MetaType::Ctor<std::string_view>{} };

	//type.AddField(&Ability::mOnAbilityActivateScript, "mOnAbilityActivateScript");
	//type.AddField(&Ability::mIconTexture, "mIconTexture");
	//type.AddField(&Ability::mDescription, "mDescription");
	//type.AddField(&Ability::mGlobalCooldown, "mGlobalCooldown");
	//type.AddField(&Ability::mRequirementType, "mRequirementType");
	//type.AddField(&Ability::mRequirementToUse, "mRequirementToUse");
	//type.AddField(&Ability::mCharges, "mCharges");

	type.AddField(&Weapon::mTimeBetweenShots, "mTimeBetweenShots");
	type.AddField(&Weapon::mFireSpeed, "mFireSpeed");
	type.AddField(&Weapon::mReloadSpeed, "mReloadSpeed");
	type.AddField(&Weapon::mProjectileCount, "mProjectileCount");
	type.AddField(&Weapon::mSpread, "mSpread");

	type.AddField(&Weapon::mEffects, "mEffects");
	type.AddField(&Weapon::mProjectileSize, "mProjectileSize");
	type.AddField(&Weapon::mProjectileSpeed, "mProjectileSpeed");
	type.AddField(&Weapon::mProjectileRange, "mProjectileRange");
	type.AddField(&Weapon::mKnockback, "mKnockback");
	type.AddField(&Weapon::mPiercing, "mPiercing");

	type.AddFunc([](const AssetHandle<Weapon>& weapon) -> float
		{
			if (weapon == nullptr)
			{
				return 0.0f;
			}

			return weapon->mTimeBetweenShots;
		},
		"GetTimeBetweenShots", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	ReflectAssetType<Weapon>(type);
	return type;
}
