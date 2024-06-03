#pragma once
#include "Ability.h"
#include "Components/Abilities/AbilityEffectsComponent.h"


namespace CE
{
	class AbilityFunctionality;
	class WeaponEditorSystem;
	struct WeaponInstance;

	class Weapon :
		public Ability
	{
	public:
		Weapon(std::string_view name);
		Weapon(AssetLoadInfo& loadInfo);

		Weapon(Weapon&&) noexcept = default;
		Weapon(const Weapon&) = default;

		Weapon& operator=(Weapon&&) = delete;
		Weapon& operator=(const Weapon&) = default;

		// weapon
		float mShotDelay{};
		float mFireSpeed = 1.f;
		float mReloadSpeed = 1.f;
		int mProjectileCount = 1;
		float mSpread{};

		// projectile
		std::vector<AbilityEffect> mEffects{};
		float mProjectileSize = 1.f;
		float mProjectileSpeed = 1.f;
		float mProjectileRange = 1.f;
		float mKnockback{};
		int mPierceCount{};

	private:
		void OnSave(AssetSaveInfo& saveInfo) const override;

		friend ReflectAccess;
		friend WeaponEditorSystem;
		static MetaType Reflect();
		REFLECT_AT_START_UP(Weapon);
	};
}

