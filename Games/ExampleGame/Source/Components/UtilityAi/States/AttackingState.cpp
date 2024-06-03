#include "Precomp.h"
#include "Components/UtililtyAi/States/AttackingState.h"

#include "BehaviourStuff.h"
#include "Components/TransformComponent.h"
#include "Components/Abilities/CharacterComponent.h"
#include "Components/Abilities/AbilitiesOnCharacterComponent.h"
#include "Systems/AbilitySystem.h"
#include "Meta/MetaType.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Components/AnimationRootComponent.h"
#include "Components/PlayerComponent.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Assets/Animation/Animation.h"

void Game::AttackingState::OnAITick(CE::World& world, entt::entity owner, float) const
{
	auto* animationRootComponent = world.GetRegistry().TryGet<CE::AnimationRootComponent>(owner);

	if (animationRootComponent != nullptr)
	{
		animationRootComponent->SwitchAnimation(world.GetRegistry(), mAttackingAnimation, 0.0f);
	}

	Game::ExecuteEnemyAbility(world, owner);

	auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);

	if (physicsBody2DComponent == nullptr)
	{
		LOG(LogAI, Warning, "A PhysicsBody2D component is needed to run the Attack State!");
		return;
	}

	physicsBody2DComponent->mLinearVelocity = { 0,0 };
}

float Game::AttackingState::OnAIEvaluate(const CE::World& world, entt::entity owner) const
{
	const auto score = GetBestScoreBasedOnDetection(world, owner, mRadius);
	return score;
}

CE::MetaType Game::AttackingState::Reflect()
{
	auto type = CE::MetaType{CE::MetaType::T<AttackingState>{}, "AttackingState"};
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddField(&AttackingState::mRadius, "mRadius").GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &AttackingState::OnAITick);
	BindEvent(type, CE::sAIEvaluateEvent, &AttackingState::OnAIEvaluate);

	type.AddField(&AttackingState::mAttackingAnimation, "mAttackingAnimation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<AttackingState>(type);
	return type;
}
