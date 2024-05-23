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

	*serializedTimeBetweenShots >> mTimeBetweenShots;
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
	obj.AddGSONMember("PierceCount") << mPierceCount;

	obj.SaveToBinary(saveInfo.GetStream());
}

CE::MetaType CE::Weapon::Reflect()
{
	MetaType type = MetaType{ MetaType::T<Weapon>{}, "Weapon", MetaType::Base<Ability>{}, MetaType::Ctor<AssetLoadInfo&>{}, MetaType::Ctor<std::string_view>{} };

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
	type.AddField(&Weapon::mPierceCount, "mPierceCount");

	type.AddFunc([](const AssetHandle<Weapon>& weapon) -> float
		{
			if (weapon == nullptr)
			{
				return 0.0f;
			}
			return weapon->mTimeBetweenShots;
		},
		"GetTimeBetweenShots", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](AssetHandle<Weapon>& weapon, float v)
		{
			if (weapon == nullptr)
			{
				return;
			}
			const_cast<Weapon&>(*weapon).mTimeBetweenShots = v;
		},
		"SetTimeBetweenShots", MetaFunc::ExplicitParams<AssetHandle<Weapon>&, float>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& weapon) -> float
		{
			if (weapon == nullptr)
			{
				return 0.0f;
			}
			return weapon->mFireSpeed;
		},
		"GetFireSpeed", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](AssetHandle<Weapon>& weapon, float v)
		{
			if (weapon == nullptr)
			{
				return;
			}
			const_cast<Weapon&>(*weapon).mFireSpeed = v;
		},
		"SetFireSpeed", MetaFunc::ExplicitParams<AssetHandle<Weapon>&, float>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& weapon) -> float
		{
			if (weapon == nullptr)
			{
				return 0.0f;
			}
			return weapon->mReloadSpeed;
		},
		"GetReloadSpeed", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](AssetHandle<Weapon>& weapon, float v)
		{
			if (weapon == nullptr)
			{
				return;
			}
			const_cast<Weapon&>(*weapon).mReloadSpeed = v;
		},
		"SetReloadSpeed", MetaFunc::ExplicitParams<AssetHandle<Weapon>&, float>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& weapon) -> int
		{
			if (weapon == nullptr)
			{
				return 0;
			}
			return weapon->mProjectileCount;
		},
		"GetProjectileCount", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](AssetHandle<Weapon>& weapon, int v)
		{
			if (weapon == nullptr)
			{
				return;
			}
			const_cast<Weapon&>(*weapon).mProjectileCount = v;
		},
		"SetProjectileCount", MetaFunc::ExplicitParams<AssetHandle<Weapon>&, int>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& weapon) -> float
		{
			if (weapon == nullptr)
			{
				return 0.0f;
			}
			return weapon->mSpread;
		},
		"GetSpread", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](AssetHandle<Weapon>& weapon, float v)
		{
			if (weapon == nullptr)
			{
				return;
			}
			const_cast<Weapon&>(*weapon).mSpread = v;
		},
		"SetSpread", MetaFunc::ExplicitParams<AssetHandle<Weapon>&, float>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& weapon) -> std::vector<AbilityEffect>
		{
			if (weapon == nullptr)
			{
				return {};
			}
			return weapon->mEffects;
		},
		"GetEffects", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](AssetHandle<Weapon>& weapon, AbilityEffect effect, int index)
		{
			int numEffects = static_cast<int>(weapon->mEffects.size());
			if (weapon == nullptr || numEffects <= index)
			{
				return;
			}
			const_cast<Weapon&>(*weapon).mEffects[index] = effect;
		},
		"SetEffectAtIndex", MetaFunc::ExplicitParams<AssetHandle<Weapon>&, AbilityEffect, int>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& weapon) -> float
		{
			if (weapon == nullptr)
			{
				return 0.0f;
			}
			return weapon->mProjectileSize;
		},
		"GetProjectileSize", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](AssetHandle<Weapon>& weapon, float v)
		{
			if (weapon == nullptr)
			{
				return;
			}
			const_cast<Weapon&>(*weapon).mProjectileSize = v;
		},
		"SetProjectileSize", MetaFunc::ExplicitParams<AssetHandle<Weapon>&, float>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& weapon) -> float
		{
			if (weapon == nullptr)
			{
				return 0.0f;
			}
			return weapon->mProjectileSpeed;
		},
		"GetProjectileSpeed", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](AssetHandle<Weapon>& weapon, float v)
		{
			if (weapon == nullptr)
			{
				return;
			}
			const_cast<Weapon&>(*weapon).mProjectileSpeed = v;
		},
		"SetProjectileSpeed", MetaFunc::ExplicitParams<AssetHandle<Weapon>&, float>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& weapon) -> float
		{
			if (weapon == nullptr)
			{
				return 0.0f;
			}
			return weapon->mProjectileRange;
		},
		"GetProjectileRange", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](AssetHandle<Weapon>& weapon, float v)
		{
			if (weapon == nullptr)
			{
				return;
			}
			const_cast<Weapon&>(*weapon).mProjectileRange = v;
		},
		"SetProjectileRange", MetaFunc::ExplicitParams<AssetHandle<Weapon>&, float>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& weapon) -> float
		{
			if (weapon == nullptr)
			{
				return 0.0f;
			}
			return weapon->mKnockback;
		},
		"GetKnockback", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](AssetHandle<Weapon>& weapon, float v)
		{
			if (weapon == nullptr)
			{
				return;
			}
			const_cast<Weapon&>(*weapon).mKnockback = v;
		},
		"SetKnockback", MetaFunc::ExplicitParams<AssetHandle<Weapon>&, float>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& weapon) -> int
		{
			if (weapon == nullptr)
			{
				return 0;
			}
			return weapon->mPierceCount;
		},
		"GetPierceCount", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](AssetHandle<Weapon>& weapon, int v)
		{
			if (weapon == nullptr)
			{
				return;
			}
			const_cast<Weapon&>(*weapon).mPierceCount = v;
		},
		"SetPierceCount", MetaFunc::ExplicitParams<AssetHandle<Weapon>&, int>{}).GetProperties().Add(Props::sIsScriptableTag);

	// ability stats
	type.AddFunc([](const AssetHandle<Weapon>& weapon) -> float
		{
			if (weapon == nullptr)
			{
				return 0.f;
			}
			return weapon->mRequirementToUse;
		},
		"GetReloadTime", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](AssetHandle<Weapon>& weapon, float v)
		{
			if (weapon == nullptr)
			{
				return;
			}
			const_cast<Weapon&>(*weapon).mRequirementToUse = v;
		},
		"SetReloadTime", MetaFunc::ExplicitParams<AssetHandle<Weapon>&, float>{}).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const AssetHandle<Weapon>& weapon) -> int
		{
			if (weapon == nullptr)
			{
				return 0;
			}
			return weapon->mCharges;
		},
		"GetAmmo", MetaFunc::ExplicitParams<const AssetHandle<Weapon>&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](AssetHandle<Weapon>& weapon, int v)
		{
			if (weapon == nullptr)
			{
				return;
			}
			const_cast<Weapon&>(*weapon).mCharges = v;
		},
		"SetAmmo", MetaFunc::ExplicitParams<AssetHandle<Weapon>&, int>{}).GetProperties().Add(Props::sIsScriptableTag);

	ReflectAssetType<Weapon>(type);
	return type;
}
