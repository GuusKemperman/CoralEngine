#pragma once
#include "Meta/Fwd/MetaReflectFwd.h"

namespace Engine
{
	class CharacterComponent;
	class World;

	class AbilityFunctionality
	{
	public:
		enum Stat
		{
			Health,
			MovementSpeed
		};

		enum FlatOrPercentage
		{
			Flat,
			Percentage
		};

		enum IncreaseOrDecrease
		{
			Decrease,
			Increase
		};

		static void ApplyInstantEffect(World& world, entt::entity affectedEntity, Stat stat = Stat::Health, float amount = 0.f, FlatOrPercentage flatOrPercentage = FlatOrPercentage::Flat, IncreaseOrDecrease increaseOrDecrease = IncreaseOrDecrease::Decrease);

	private:
		static std::pair<float&, float&> GetStat(Stat stat, CharacterComponent& characterComponent);

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AbilityFunctionality);
	};
}
