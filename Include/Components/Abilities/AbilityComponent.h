#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
	class AbilityComponent
	{
	public:
		std::string mIconTextureName{};
		std::string mDescription{};

		enum Target
		{
			Hostile,
			Self,
			Friendly,
			SelfAndFriendly
		}mTarget{};

		bool mGlobalCooldown = true; // whether this ability takes into account the global cooldown

		enum RequirementType
		{
			Cooldown,
			Kills
		}mRequirementType{};

		float mRequirementToUse{};
		float mRequirementCounter{};

		int mCharges = 1;
		int mCurrentCharges{};

		entt::entity mCastByPlayer{};
		std::vector<entt::entity> mHitPlayers{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AbilityComponent);
	};
}

template<>
struct Reflector<Engine::AbilityComponent::Target>
{
	static Engine::MetaType Reflect();
	static constexpr bool sIsSpecialized = true;
}; REFLECT_AT_START_UP(Target, Engine::AbilityComponent::Target);

template<>
struct Engine::EnumStringPairsImpl<Engine::AbilityComponent::Target>
{
	static constexpr EnumStringPairs<AbilityComponent::Target, 4> value = {
		EnumStringPair<AbilityComponent::Target>{ AbilityComponent::Target::Hostile, "Hostile" },
		{ AbilityComponent::Target::Self, "Self" },
		{ AbilityComponent::Target::Friendly, "Friendly" },
		{ AbilityComponent::Target::SelfAndFriendly, "SelfAndFriendly" },
	};
};

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
		{ AbilityComponent::RequirementType::Kills, "Kills" },
	};
};