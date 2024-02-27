#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
	class PhysicsSystem2D;

	/// <summary>
	/// Stores the details of a single collision.
	/// </summary>
	struct CollisionData
	{
		/// The ID of the first entity involved in the collision.
		entt::entity mEntity1{};

		/// The ID of the second entity involved in the collision.
		entt::entity mEntity2{};

		/// The normal vector on the point of contact, pointing away from entity2's physics body.
		glm::vec2 mNormal{};

		/// The penetration depth of the two physics bodies
		/// (before they were displaced to resolve overlap).
		float mDepth{};

		/// The approximate point of contact of the collision, in world coordinates.
		glm::vec2 mContactPoint{};
	};

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
		explicit PhysicsBody2DComponent(float mass, float restitution, glm::vec2 position)
			: mRestitution(restitution), mPosition(position)
		{
			mInvMass = mass == 0.f ? 0.f : (1.f / mass);
		}

		MotionType mMotionType{};
		float mInvMass = 1.f;
		float mRestitution = 1.f;
		glm::vec2 mPosition{};
		glm::vec2 mLinearVelocity{};
		glm::vec2 mForce{};

		inline void AddForce(const glm::vec2& force)
		{
			if (mMotionType == MotionType::Dynamic) mForce += force;
		}
		inline void ApplyImpulse(const glm::vec2& imp)
		{
			if (mMotionType == MotionType::Dynamic) mLinearVelocity += imp * mInvMass;
		}

		std::vector<CollisionData> mCollisions = {};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(PhysicsBody2DComponent);

		friend PhysicsSystem2D;

		void AddCollisionData(const CollisionData& data) { mCollisions.push_back(data); }
		void ClearCollisionData() { mCollisions.clear(); }
		
		void ClearForces() { mForce = {}; }

		void Update(float dt)
		{
			ClearCollisionData();
			if (mMotionType == MotionType::Dynamic) mLinearVelocity += mForce * mInvMass * dt;
			mPosition += mLinearVelocity * dt;
		}
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
