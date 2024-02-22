#include "Precomp.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"

Engine::MetaType Engine::PhysicsBody2DComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<PhysicsBody2DComponent>{}, "PhysicsBody2DComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&PhysicsBody2DComponent::mInvMass, "mInvMass").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&PhysicsBody2DComponent::mRestitution, "mRestitution").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&PhysicsBody2DComponent::mPosition, "mPosition").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&PhysicsBody2DComponent::mLinearVelocity, "mLinearVelocity").GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<PhysicsBody2DComponent>(metaType);

	return metaType;
}
