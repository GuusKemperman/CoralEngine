#pragma once
#include "Assets/Asset.h"

namespace Engine
{
	class Texture;
	class Script;

	class Ability :
		public Asset
	{
	public:
        Ability(std::string_view name);
        Ability(AssetLoadInfo& loadInfo);

		bool operator==(const Ability& other) const;
		bool operator!=(const Ability& other) const;

		enum RequirementType
		{
			Cooldown,
			Mana // can be based on kills or some other criteria
		};

	private:
        void OnSave(AssetSaveInfo& saveInfo) const override;

		std::shared_ptr<const Texture> mIconTexture{};
		std::string mDescription{};

		bool mGlobalCooldown = true; // whether this ability takes into account the global cooldown

		RequirementType mRequirementType{};

		float mRequirementToUse{};
		int mCharges = 1; // how many times the ability can be used before going back on cooldown

        friend ReflectAccess;
        static MetaType Reflect();
        REFLECT_AT_START_UP(Ability);
	};
}

template<>
struct Reflector<Engine::Ability::RequirementType>
{
	static Engine::MetaType Reflect();
	static constexpr bool sIsSpecialized = true;
}; REFLECT_AT_START_UP(RequirementType, Engine::Ability::RequirementType);

template<>
struct Engine::EnumStringPairsImpl<Engine::Ability::RequirementType>
{
	static constexpr EnumStringPairs<Ability::RequirementType, 2> value = {
		EnumStringPair<Ability::RequirementType>{ Ability::RequirementType::Cooldown, "Cooldown" },
		{ Ability::RequirementType::Mana, "Mana" },
	};
};
