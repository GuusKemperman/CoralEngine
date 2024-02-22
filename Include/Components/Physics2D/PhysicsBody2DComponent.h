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
		entt::entity mEntity1;

		/// The ID of the second entity involved in the collision.
		entt::entity mEntity2;

		/// The normal vector on the point of contact, pointing away from entity2's physics body.
		glm::vec2 mNormal;

		/// The penetration depth of the two physics bodies
		/// (before they were displaced to resolve overlap).
		float mDepth;

		/// The approximate point of contact of the collision, in world coordinates.
		glm::vec2 mContactPoint;
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

		//Type mType;
		float mInvMass = 1.f;
		float mRestitution = 1.f;
		glm::vec2 mPosition = { 0.f, 0.f };
		glm::vec2 mLinearVelocity = { 0.f, 0.f };
		//glm::vec2 mForce = { 0.f, 0.f };

		std::vector<CollisionData> mCollisions = {};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(PhysicsBody2DComponent);

		friend PhysicsSystem2D;
		
		//void ClearForces() { mForce = { 0.f, 0.f }; }

		void Update(float dt)
		{
			//if (mType == Dynamic) mLinearVelocity += mForce * mInvMass * dt;
			mPosition += mLinearVelocity * dt;
		}
	};
}