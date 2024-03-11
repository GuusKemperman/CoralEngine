#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
	class CharacterComponent
	{
	public:
		int mTeamId = -1; // for targeting

		float mGlobalCooldown = 0.25f;
		float mGlobalCooldownTimer = mGlobalCooldown;

		float mBaseHealth = 100.f;
		float mCurrentHealth = mBaseHealth;

		float mBaseMovementSpeed = 10.f;
		float mCurrentMovementSpeed = mBaseMovementSpeed;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(CharacterComponent);
	};
}
