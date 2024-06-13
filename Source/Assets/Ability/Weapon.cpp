#include "Precomp.h"
#include "Assets/Ability/Weapon.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Assets/Core/AssetLoadInfo.h"
#include "Assets/Core/AssetSaveInfo.h"
#include "Utilities/Reflect/ReflectAssetType.h"
#include "Assets/Texture.h"
#include "Assets/Script.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"

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
	const BinaryGSONMember* serializedPierceCount = obj.TryGetGSONMember("PierceCount");

	const BinaryGSONMember* serializedShootOnRelease = obj.TryGetGSONMember("ShootOnRelease");

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
		|| serializedPierceCount == nullptr
		|| serializedShootOnRelease == nullptr)
	{
		LOG(LogAssets, Error, "Could not load weapon {}, as there were missing values.", GetName());
		return;
	}

	*serializedTimeBetweenShots >> mShotDelay;
	*serializedFireSpeed >> mFireSpeed;
	*serializedReloadSpeed >> mReloadSpeed;
	*serializedProjectileCount >> mProjectileCount;
	*serializedSpread >> mSpread;

	*serializedEffects >> mEffects;
	*serializedProjectileSize >> mProjectileSize;
	*serializedProjectileSpeed >> mProjectileSpeed;
	*serializedProjectileRange >> mProjectileRange;
	*serializedKnockback >> mKnockback;
	*serializedPierceCount >> mPierceCount;

	*serializedShootOnRelease >> mShootOnRelease;

	const BinaryGSONMember* shootingSlowDown = obj.TryGetGSONMember("ShootingSlowdown");

	if (shootingSlowDown != nullptr)
	{
		*shootingSlowDown >> mShootingSlowdown;
	}
}

void CE::Weapon::OnSave(AssetSaveInfo& saveInfo) const
{
	Ability::OnSave(saveInfo);

	BinaryGSONObject obj{};

	obj.AddGSONMember("TimeBetweenShots") << mShotDelay;
	obj.AddGSONMember("FireSpeed") << mFireSpeed;
	obj.AddGSONMember("ReloadSpeed") << mReloadSpeed;
	obj.AddGSONMember("ProjectileCount") << mProjectileCount;
	obj.AddGSONMember("Spread") << mSpread;
	obj.AddGSONMember("ShootingSlowdown") << mShootingSlowdown;

	obj.AddGSONMember("Effects") << mEffects;
	obj.AddGSONMember("ProjectileSize") << mProjectileSize;
	obj.AddGSONMember("ProjectileSpeed") << mProjectileSpeed;
	obj.AddGSONMember("ProjectileRange") << mProjectileRange;
	obj.AddGSONMember("Knockback") << mKnockback;
	obj.AddGSONMember("PierceCount") << mPierceCount;

	obj.AddGSONMember("ShootOnRelease") << mShootOnRelease;

	obj.SaveToBinary(saveInfo.GetStream());
}

CE::MetaType CE::Weapon::Reflect()
{
	MetaType type = MetaType{ MetaType::T<Weapon>{}, "Weapon", MetaType::Base<Ability>{}, MetaType::Ctor<AssetLoadInfo&>{}, MetaType::Ctor<std::string_view>{} };
	type.GetProperties().Add(Props::sIsScriptableTag);

	type.AddField(&Weapon::mShotDelay, "mShotDelay").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&Weapon::mFireSpeed, "mFireSpeed").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&Weapon::mReloadSpeed, "mReloadSpeed").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&Weapon::mProjectileCount, "mProjectileCount").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&Weapon::mSpread, "mSpread").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&Weapon::mShootingSlowdown, "mShootingSlowdown").GetProperties().Add(Props::sIsScriptableTag);

	type.AddField(&Weapon::mEffects, "mEffects").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&Weapon::mProjectileSize, "mProjectileSize").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&Weapon::mProjectileSpeed, "mProjectileSpeed").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&Weapon::mProjectileRange, "mProjectileRange").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&Weapon::mKnockback, "mKnockback").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&Weapon::mPierceCount, "mPierceCount").GetProperties().Add(Props::sIsScriptableTag);

	type.AddField(&Weapon::mShootOnRelease, "mShootOnRelease").GetProperties().Add(Props::sIsScriptableTag);

	// Weapon.
	type.AddFunc([](Weapon& weapon) -> std::vector<AbilityEffect>&
		{
			return weapon.mEffects;
		},
		"GetEffects (Weapon Instance)", MetaFunc::ExplicitParams<Weapon&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](Weapon& weapon, AbilityEffect effect, int index)
		{
			if (index >= static_cast<int>(weapon.mEffects.size()) || index < 0)
			{
				LOG(LogAbilitySystem, Error, "Index {} out of range for weapon {}.", index, weapon.GetName());
				return;
			}
			weapon.mEffects[index] = effect;
		},
		"SetEffectAtIndex (Weapon Instance)", MetaFunc::ExplicitParams<Weapon&, AbilityEffect, int>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const Weapon& weapon) -> float
		{
			return weapon.mRequirementToUse;
		},
		"GetReloadTime (Weapon Instance)", MetaFunc::ExplicitParams<const Weapon&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](Weapon& weapon, float v)
		{
			weapon.mRequirementToUse = v;
		},
		"SetReloadTime (Weapon Instance)", MetaFunc::ExplicitParams<Weapon&, float>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const Weapon& weapon) -> int
		{
			return weapon.mCharges;
		},
		"GetAmmo (Weapon Instance)", MetaFunc::ExplicitParams<const Weapon&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](Weapon& weapon, int v)
		{
			weapon.mCharges = v;
		},
		"SetAmmo (Weapon Instance)", MetaFunc::ExplicitParams<Weapon&, int>{}).GetProperties().Add(Props::sIsScriptableTag);

	// Weapon asset.
	// Non-inherited members.
	type.AddFunc([](const AssetHandle<Weapon>& weapon) -> float
		{
			if (weapon == nullptr)
			{
				LOG(LogAbilitySystem, Error, "GetShotDelay - weapon was NULL.");
				return {};
			}
			return weapon->mShotDelay;
		},
		"GetShotDelay", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& weapon) -> float
		{
			if (weapon == nullptr)
			{
				LOG(LogAbilitySystem, Error, "GetFireSpeed - weapon was NULL.");
				return {};
			}
			return weapon->mFireSpeed;
		},
		"GetFireSpeed", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& weapon) -> float
		{
			if (weapon == nullptr)
			{
				LOG(LogAbilitySystem, Error, "GetReloadSpeed - weapon was NULL.");
				return {};
			}
			return weapon->mReloadSpeed;
		},
		"GetReloadSpeed", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& weapon) -> int
		{
			if (weapon == nullptr)
			{
				LOG(LogAbilitySystem, Error, "GetProjectileCount - weapon was NULL.");
				return {};
			}
			return weapon->mProjectileCount;
		},
		"GetProjectileCount", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& weapon) -> float
		{
			if (weapon == nullptr)
			{
				LOG(LogAbilitySystem, Error, "GetSpread - weapon was NULL.");
				return {};
			}
			return weapon->mSpread;
		},
		"GetSpread", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& weapon) -> float
		{
			if (weapon == nullptr)
			{
				LOG(LogAbilitySystem, Error, "GetShootingSlowdown - weapon was NULL.");
				return {};
			}
			return weapon->mShootingSlowdown;
		},
		"GetShootingSlowdown", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& weapon) -> std::vector<AbilityEffect>
		{
			if (weapon == nullptr)
			{
				LOG(LogAbilitySystem, Error, "GetEffects - weapon was NULL.");
				return {};
			}
			return weapon->mEffects;
		},
		"GetEffects", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& weapon) -> float
		{
			if (weapon == nullptr)
			{
				LOG(LogAbilitySystem, Error, "GetProjectileSize - weapon was NULL.");
				return {};
			}
			return weapon->mProjectileSize;
		},
		"GetProjectileSize", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& weapon) -> float
		{
			if (weapon == nullptr)
			{
				LOG(LogAbilitySystem, Error, "GetProjectileSpeed - weapon was NULL.");
				return {};
			}
			return weapon->mProjectileSpeed;
		},
		"GetProjectileSpeed", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& weapon) -> float
		{
			if (weapon == nullptr)
			{
				LOG(LogAbilitySystem, Error, "GetProjectileRange - weapon was NULL.");
				return {};
			}
			return weapon->mProjectileRange;
		},
		"GetProjectileRange", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& weapon) -> float
		{
			if (weapon == nullptr)
			{
				LOG(LogAbilitySystem, Error, "GetKnockback - weapon was NULL.");
				return {};
			}
			return weapon->mKnockback;
		},
		"GetKnockback", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& weapon) -> int
		{
			if (weapon == nullptr)
			{
				LOG(LogAbilitySystem, Error, "GetPierceCount - weapon was NULL.");
				return {};
			}
			return weapon->mPierceCount;
		},
		"GetPierceCount", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	// Ability-inherited members.
	type.AddFunc([](const AssetHandle<Weapon>& weapon) -> AssetHandle<Script>
		{
			if (weapon == nullptr)
			{
				LOG(LogAbilitySystem, Error, "GetOnAbilityActivateScript - weapon was NULL.");
				return {};
			}
			return weapon->mOnAbilityActivateScript;
		},
		"GetOnAbilityActivateScript", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& weapon) -> AssetHandle<Texture>
		{
			if (weapon == nullptr)
			{
				LOG(LogAbilitySystem, Error, "GetIconTexture - weapon was NULL.");
				return {};
			}
			return weapon->mIconTexture;
		},
		"GetIconTexture", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& weapon) -> std::string
		{
			if (weapon == nullptr)
			{
				LOG(LogAbilitySystem, Error, "GetDescription - weapon was NULL.");
				return {};
			}
			return weapon->mDescription;
		},
		"GetDescription", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& weapon) -> bool
		{
			if (weapon == nullptr)
			{
				LOG(LogAbilitySystem, Error, "GetGlobalCooldown - weapon was NULL.");
				return {};
			}
			return weapon->mGlobalCooldown;
		},
		"GetGlobalCooldown", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	// Skipping mRequirementType because it is not relevant for the weapons.

	type.AddFunc([](const AssetHandle<Weapon>& weapon) -> float
		{
			if (weapon == nullptr)
			{
				LOG(LogAbilitySystem, Error, "GetReloadTime - weapon was NULL.");
				return {};
			}
			return weapon->mRequirementToUse;
		},
		"GetReloadTime", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& weapon) -> int
		{
			if (weapon == nullptr)
			{
				LOG(LogAbilitySystem, Error, "GetAmmo - weapon was NULL.");
				return {};
			}
			return weapon->mCharges;
		},
		"GetAmmo", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	ReflectAssetType<Weapon>(type);
	return type;
}
