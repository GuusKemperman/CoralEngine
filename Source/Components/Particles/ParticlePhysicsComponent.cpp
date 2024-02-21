#include "Precomp.h"
#include "Components/Particles/ParticlePhysicsComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"

Engine::MetaType Engine::ParticlePhysicsComponent::Reflect()
{
	MetaType type = MetaType{ MetaType::T<ParticlePhysicsComponent>{}, "ParticlePhysicsComponent" };
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	type.AddField(&ParticlePhysicsComponent::mMinInitialVelocity, "mMinInitialVelocity").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticlePhysicsComponent::mMaxInitialVelocity, "mMaxInitialVelocity").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticlePhysicsComponent::mMinInitialRotationalVelocity, "mMinInitialRotationalVelocity").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticlePhysicsComponent::mMaxInitialRotationalVelocity, "mMaxInitialRotationalVelocity").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticlePhysicsComponent::mMinMass, "mMinMass").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticlePhysicsComponent::mMaxMass, "mMaxMass").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticlePhysicsComponent::mGravity, "mGravity").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticlePhysicsComponent::mFloorHeight, "mFloorHeight").GetProperties().Add(Props::sIsScriptableTag);
	ReflectComponentType<ParticlePhysicsComponent>(type);
	return type;
}
