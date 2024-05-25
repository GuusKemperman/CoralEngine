#include "Precomp.h"
#include "Components/UtililtyAi/States/DanceState.h"

#include "Components/Abilities/CharacterComponent.h"
#include "Meta/MetaType.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Assets/Animation/Animation.h"
#include "Components/AnimationRootComponent.h"
#include "Components/PlayerComponent.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"

void Game::DanceState::OnAITick(CE::World& world, const entt::entity owner, const float)
{
	auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);

	if (physicsBody2DComponent == nullptr)
	{
		return;
	}

	physicsBody2DComponent->mLinearVelocity = { 0,0 };
}

float Game::DanceState::OnAiEvaluate(const CE::World& world, entt::entity)
{
	const entt::entity entityId = world.GetRegistry().View<CE::PlayerComponent>().front();

	if (entityId == entt::null)
	{
		return 1.0f;
	}

	const auto characterComponent = world.GetRegistry().TryGet<CE::CharacterComponent>(entityId);

	if (characterComponent->mCurrentHealth <= 0.f)
	{
		return 1.0f;
	}

	return 0.f;
}

void Game::DanceState::OnAIStateEnterEvent(CE::World& world, entt::entity owner) const
{
	auto* animationRootComponent = world.GetRegistry().TryGet<CE::AnimationRootComponent>(owner);

	if (animationRootComponent != nullptr)
	{
		animationRootComponent->SwitchAnimation(world.GetRegistry(), mDanceAnimation, 0.0f);
	}
}

CE::MetaType Game::DanceState::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<DanceState>{}, "DanceState" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &DanceState::OnAITick);
	BindEvent(type, CE::sAIEvaluateEvent, &DanceState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &DanceState::OnAIStateEnterEvent);

	type.AddField(&DanceState::mDanceAnimation, "mDanceAnimation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<DanceState>(type);
	return type;
}
