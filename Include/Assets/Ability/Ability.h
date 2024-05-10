#pragma once
#include "Assets/Asset.h"
#include "Assets/Core/AssetHandle.h"

namespace CE
{
	class AbilitiesOnCharacterComponent;
	struct AbilityInstance;
	class AbilitySystem;
	class Texture;
	class Script;

	class Ability :
		public Asset
	{
	public:
        Ability(std::string_view name);
        Ability(AssetLoadInfo& loadInfo);

		enum RequirementType
		{
			Cooldown,
			Mana // can be based on kills or some other criteria
		};

		const AssetHandle<Texture>& GetIconTexture() const { return mIconTexture; }

	private:
		friend AbilitySystem;
		friend AbilityInstance;
		friend AbilitiesOnCharacterComponent;

        void OnSave(AssetSaveInfo& saveInfo) const override;

		AssetHandle<Script> mOnAbilityActivateScript{};

		AssetHandle<Texture> mIconTexture{};
		std::string mDescription{};

		/**
		*@brief Indicates whether this ability takes into account the GDC (global cooldown).
		* Every player has a GDC, so regardless of if an ability is off its own internal cooldown,
		* it still could not be activated if this variable is set to true. A GDC ensures abilities cannot be spammed.
		*/
		bool mGlobalCooldown = true;

		RequirementType mRequirementType{};

		float mRequirementToUse{};
		int mCharges = 1; // how many times the ability can be used before going back on cooldown

        friend ReflectAccess;
        static MetaType Reflect();
        REFLECT_AT_START_UP(Ability);
	};
}

template<>
struct Reflector<CE::Ability::RequirementType>
{
	static CE::MetaType Reflect();
	static constexpr bool sIsSpecialized = true;
}; REFLECT_AT_START_UP(RequirementType, CE::Ability::RequirementType);

template<>
struct CE::EnumStringPairsImpl<CE::Ability::RequirementType>
{
	static constexpr EnumStringPairs<Ability::RequirementType, 2> value = {
		EnumStringPair<Ability::RequirementType>{ Ability::RequirementType::Cooldown, "Cooldown" },
		{ Ability::RequirementType::Mana, "Mana" },
	};
};
