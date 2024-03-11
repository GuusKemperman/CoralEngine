#pragma once
#include <forward_list>

#include "Meta/MetaReflect.h"

namespace Engine
{
	class World;
	class PhysicsSystem2D;

	enum class MotionType
	{
		// Indicates a physics body that does not move and is not affected by forces, as if it has infinite mass.
		Static,

		// Indicates a physics body that can move under the influence of forces.
		Dynamic,

		// Indicates a physics body that is not affected by forces. It moves purely according to its velocity.
		Kinematic
	};

	class PhysicsBody2DComponent
	{
	public:
		PhysicsBody2DComponent() = default;
		explicit PhysicsBody2DComponent(float mass, float restitution)
			: mRestitution(restitution)
		{
			mInvMass = mass == 0.f ? 0.f : (1.f / mass);
		}

		MotionType mMotionType{};
		float mInvMass = 1.f;
		float mRestitution = 1.f;
		glm::vec2 mLinearVelocity{};
		glm::vec2 mForce{};

		void AddForce(const glm::vec2& force)
		{
			if (mMotionType == MotionType::Dynamic) mForce += force;
		}

		void ApplyImpulse(const glm::vec2& imp)
		{
			if (mMotionType == MotionType::Dynamic) mLinearVelocity += imp * mInvMass;
		}


	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(PhysicsBody2DComponent);

		friend PhysicsSystem2D;

		void ClearForces() { mForce = {}; }
	};
}

template<>
struct Reflector<Engine::MotionType>
{
	static Engine::MetaType Reflect();
	static constexpr bool sIsSpecialized = true;
}; REFLECT_AT_START_UP(MotionType, Engine::MotionType);

template<>
struct Engine::EnumStringPairsImpl<Engine::MotionType>
{
	static constexpr EnumStringPairs<MotionType, 3> value = {
		EnumStringPair<MotionType>{ MotionType::Static, "Static" },
		{ MotionType::Dynamic, "Dynamic" },
		{ MotionType::Kinematic, "Kinematic" },
	};
};
