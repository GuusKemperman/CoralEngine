#include "Precomp.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Utilities/Reflect/ReflectFieldType.h"

Engine::MetaType Engine::PhysicsBody2DComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<PhysicsBody2DComponent>{}, "PhysicsBody2DComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&PhysicsBody2DComponent::mMotionType, "mMotionType").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&PhysicsBody2DComponent::mInvMass, "mInvMass").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&PhysicsBody2DComponent::mRestitution, "mRestitution").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&PhysicsBody2DComponent::mPosition, "mPosition").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&PhysicsBody2DComponent::mLinearVelocity, "mLinearVelocity").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&PhysicsBody2DComponent::mForce, "mForce").GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<PhysicsBody2DComponent>(metaType);

	return metaType;
}

Engine::MetaType Reflector<Engine::MotionType>::Reflect()
{
	using namespace Engine;
	using T = MotionType;
	MetaType type{ MetaType::T<T>{}, "MotionType" };

	type.GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	ReflectFieldType<T>(type);

	return type;
}
