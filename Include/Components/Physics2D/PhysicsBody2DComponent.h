#pragma once
#include "BasicDataTypes/CollisionRules.h"
#include "Meta/MetaReflect.h"
#include "Utilities/Events.h"

namespace CE
{
	class World;
	class PhysicsSystem;


	struct OnCollisionEntry :
		EventType<OnCollisionEntry, void(entt::entity, float, glm::vec2, glm::vec2)>
	{
		OnCollisionEntry() :
			EventType("OnCollisionEntry", "CollidedWith", "CollisionDepth", "CollisionNormal", "PointOfContact")
		{
		}
	};

	/**
	 * \brief Called the first frame two entities are colliding
	 * \World& The world this component is in.
	 * \entt::entity The owner of this component.
	 * \entt::entity The entity this entity collided with.
	 * \float The depth of the collision, how far it penetrated
	 * \glm::vec2 The collision normal
	 * \glm::vec2 The point of contact
	 */
	inline const OnCollisionEntry sOnCollisionEntry{};

	struct OnCollisionStay :
		EventType<OnCollisionStay, void(entt::entity, float, glm::vec2, glm::vec2)>
	{
		OnCollisionStay() :
			EventType("OnCollisionStay", "CollidedWith", "CollisionDepth", "CollisionNormal", "PointOfContact")
		{
		}
	};

	/**
	 * \brief Called every frame in which two entities are colliding. ALWAYS called after at one OnCollisionEntry
	 * \World& The world this component is in.
	 * \entt::entity The owner of this component.
	 * \entt::entity The entity this entity collided with.
	 * \float The depth of the collision, how far it penetrated
	 * \glm::vec2 The collision normal
	 * \glm::vec2 The point of contact
	 */
	inline const OnCollisionStay sOnCollisionStay{};

	struct OnCollisionExit :
		EventType<OnCollisionExit, void(entt::entity, float, glm::vec2, glm::vec2)>
	{
		OnCollisionExit() :
			EventType("OnCollisionExit", "CollidedWith", "CollisionDepth", "CollisionNormal", "PointOfContact")
		{
		}
	};

	/**
	 * \brief Called the first frame that two entities who were colliding in the previous frame, but no longer are.
	 * \World& The world this component is in.
	 * \entt::entity The owner of this component.
	 * \entt::entity The entity this entity collided with.
	 * \float The depth of the collision, how far it penetrated
	 * \glm::vec2 The collision normal
	 * \glm::vec2 The point of contact
	 */
	inline const OnCollisionExit sOnCollisionExit{};

	class PhysicsBody2DComponent
	{
	public:
		PhysicsBody2DComponent() = default;
		explicit PhysicsBody2DComponent(float mass, float restitution)
			: mRestitution(restitution)
		{
			mInvMass = mass == 0.f ? 0.f : (1.f / mass);
		}

		CollisionRules mRules = CollisionPresets::sWorldDynamic.mRules;

		float mInvMass = 1.f;
		float mRestitution = 1.f;
		glm::vec2 mLinearVelocity{};
		glm::vec2 mForce{};

		/**
		 * \brief If true, this object can be moved by gravity, forces and impulses. Otherwise, it moves purely according to its velocity.
		 */
		bool mIsAffectedByForces = true;

		void AddForce(glm::vec2 force)
		{
			if (mIsAffectedByForces) mForce += force;
		}

		void ApplyImpulse(glm::vec2 imp)
		{
			if (mIsAffectedByForces) mLinearVelocity += imp * mInvMass;
		}

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(PhysicsBody2DComponent);

		friend PhysicsSystem;

		void ClearForces() { mForce = {}; }
	};
}
