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
		|| serializedPierceCount == nullptr)
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

	obj.AddGSONMember("Effects") << mEffects;
	obj.AddGSONMember("ProjectileSize") << mProjectileSize;
	obj.AddGSONMember("ProjectileSpeed") << mProjectileSpeed;
	obj.AddGSONMember("ProjectileRange") << mProjectileRange;
	obj.AddGSONMember("Knockback") << mKnockback;
	obj.AddGSONMember("PierceCount") << mPierceCount;

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

	type.AddField(&Weapon::mEffects, "mEffects").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&Weapon::mProjectileSize, "mProjectileSize").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&Weapon::mProjectileSpeed, "mProjectileSpeed").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&Weapon::mProjectileRange, "mProjectileRange").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&Weapon::mKnockback, "mKnockback").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&Weapon::mPierceCount, "mPierceCount").GetProperties().Add(Props::sIsScriptableTag);

	// Weapon pointer.
	type.AddFunc([](Weapon* weapon, int index) -> AbilityEffect
		{
			if (weapon == nullptr || index >= static_cast<int>(weapon->mEffects.size()))
			{
				return {};
			}
			return weapon->mEffects[index];
		},
		"GetEffectAtIndex (Weapon Pointer)", MetaFunc::ExplicitParams<Weapon*, int>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);
	type.AddFunc([](Weapon* weapon, AbilityEffect effect, int index)
		{
			if (weapon == nullptr || index >= static_cast<int>(weapon->mEffects.size()))
			{
				return;
			}
			weapon->mEffects[index] = effect;
		},
		"SetEffectAtIndex (Weapon Pointer)", MetaFunc::ExplicitParams<Weapon*, AbilityEffect, int>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const Weapon* weapon) -> float
		{
			if (weapon == nullptr)
			{
				return {};
			}
			return weapon->mRequirementToUse;
		},
		"GetReloadTime (Weapon Pointer)", MetaFunc::ExplicitParams<const Weapon*>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);
	type.AddFunc([](Weapon* weapon, float v)
		{
			if (weapon == nullptr)
			{
				return;
			}
			weapon->mRequirementToUse = v;
		},
		"SetReloadTime (Weapon Pointer)", MetaFunc::ExplicitParams<Weapon*, float>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](const Weapon* weapon) -> int
		{
			if (weapon == nullptr)
			{
				return {};
			}
			return weapon->mCharges;
		},
		"GetAmmo (Weapon Pointer)", MetaFunc::ExplicitParams<const Weapon*>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);
	type.AddFunc([](Weapon* weapon, int v)
		{
			if (weapon == nullptr)
			{
				return;
			}
			weapon->mCharges = v;
		},
		"SetAmmo (Weapon Pointer)", MetaFunc::ExplicitParams<Weapon*, int>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	// Weapon asset.
	// Non-inherited members.
	type.AddFunc([](const AssetHandle<Weapon>& upgrade) -> float
		{
			if (upgrade == nullptr)
			{
				return {};
			}

			return upgrade->mShotDelay;
		},
		"GetShotDelay", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& upgrade) -> float
		{
			if (upgrade == nullptr)
			{
				return {};
			}

			return upgrade->mFireSpeed;
		},
		"GetFireSpeed", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& upgrade) -> float
		{
			if (upgrade == nullptr)
			{
				return {};
			}

			return upgrade->mReloadSpeed;
		},
		"GetReloadSpeed", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& upgrade) -> int
		{
			if (upgrade == nullptr)
			{
				return {};
			}

			return upgrade->mProjectileCount;
		},
		"GetProjectileCount", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& upgrade) -> float
		{
			if (upgrade == nullptr)
			{
				return {};
			}

			return upgrade->mSpread;
		},
		"GetSpread", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& upgrade) -> std::vector<AbilityEffect>
		{
			if (upgrade == nullptr)
			{
				return {};
			}

			return upgrade->mEffects;
		},
		"GetEffects", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& upgrade, int index) -> AbilityEffect
		{
			if (upgrade == nullptr || index >= static_cast<int>(upgrade->mEffects.size()))
			{
				return {};
			}

			return upgrade->mEffects[index];
		},
		"GetEffectAtIndex", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&, int>{}, "Weapon", "Index").GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& upgrade) -> float
		{
			if (upgrade == nullptr)
			{
				return {};
			}

			return upgrade->mProjectileSize;
		},
		"GetProjectileSize", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& upgrade) -> float
		{
			if (upgrade == nullptr)
			{
				return {};
			}

			return upgrade->mProjectileSpeed;
		},
		"GetProjectileSpeed", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& upgrade) -> float
		{
			if (upgrade == nullptr)
			{
				return {};
			}

			return upgrade->mProjectileRange;
		},
		"GetProjectileRange", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& upgrade) -> float
		{
			if (upgrade == nullptr)
			{
				return {};
			}

			return upgrade->mKnockback;
		},
		"GetKnockback", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& upgrade) -> int
		{
			if (upgrade == nullptr)
			{
				return {};
			}

			return upgrade->mPierceCount;
		},
		"GetPierceCount", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	// Ability-inherited members.
	type.AddFunc([](const AssetHandle<Weapon>& upgrade) -> AssetHandle<Script>
		{
			if (upgrade == nullptr)
			{
				return nullptr;
			}

			return upgrade->mOnAbilityActivateScript;
		},
		"GetOnAbilityActivateScript", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& upgrade) -> AssetHandle<Texture>
		{
			if (upgrade == nullptr)
			{
				return nullptr;
			}

			return upgrade->mIconTexture;
		},
		"GetIconTexture", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& upgrade) -> std::string
		{
			if (upgrade == nullptr)
			{
				return {};
			}

			return upgrade->mDescription;
		},
		"GetDescription", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& upgrade) -> bool
		{
			if (upgrade == nullptr)
			{
				return {};
			}

			return upgrade->mGlobalCooldown;
		},
		"GetGlobalCooldown", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	// Skipping mRequirementType because it is not relevant for the weapons.

	type.AddFunc([](const AssetHandle<Weapon>& upgrade) -> float
		{
			if (upgrade == nullptr)
			{
				return {};
			}

			return upgrade->mRequirementToUse;
		},
		"GetReloadTime", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& upgrade) -> int
		{
			if (upgrade == nullptr)
			{
				return {};
			}

			return upgrade->mCharges;
		},
		"GetAmmo", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);

	ReflectAssetType<Weapon>(type);
	return type;
}
