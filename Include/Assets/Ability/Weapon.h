#pragma once
#include "Ability.h"
#include "Utilities/AbilityFunctionality.h"

namespace CE
{
	class WeaponEditorSystem;

	class Weapon :
		public Ability
	{
	public:
		Weapon(std::string_view name);
		Weapon(AssetLoadInfo& loadInfo);

	private:
		friend AbilitySystem;
		friend AbilityFunctionality;
		void OnSave(AssetSaveInfo& saveInfo) const override;

		// weapon
		float mTimeBetweenShots{};
		float mFireSpeed = 1.f;
		float mReloadSpeed = 1.f;
		int mProjectileCount = 1;
		float mSpread{};

		// projectile
		std::vector<AbilityFunctionality::AbilityEffect> mEffects{};
		float mProjectileSize = 1.f;
		float mProjectileSpeed = 1.f;
		float mProjectileRange = 1.f;
		float mKnockback{};
		int mPiercing{};

		friend ReflectAccess;
		friend WeaponEditorSystem;
		static MetaType Reflect();
		REFLECT_AT_START_UP(Weapon);
	};
}

