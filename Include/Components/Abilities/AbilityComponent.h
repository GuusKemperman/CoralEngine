#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
	class AbilityComponent
	{
	public:
		std::string mIconTextureName{};
		std::string mDescription{};

		bool mGlobalCooldown = true; // whether this ability takes into account the global cooldown

		enum RequirementType
		{
			Cooldown,
			Mana // can be based on kills or some other criteria
		}mRequirementType{};

		float mRequirementToUse{};
		float mRequirementCounter{};

		int mCharges = 1;
		int mCurrentCharges{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AbilityComponent);
	};
}

template<>
struct Reflector<Engine::AbilityComponent::RequirementType>
{
	static Engine::MetaType Reflect();
	static constexpr bool sIsSpecialized = true;
}; REFLECT_AT_START_UP(RequirementType, Engine::AbilityComponent::RequirementType);

template<>
struct Engine::EnumStringPairsImpl<Engine::AbilityComponent::RequirementType>
{
	static constexpr EnumStringPairs<AbilityComponent::RequirementType, 2> value = {
		EnumStringPair<AbilityComponent::RequirementType>{ AbilityComponent::RequirementType::Cooldown, "Cooldown" },
		{ AbilityComponent::RequirementType::Mana, "Mana" },
	};
};