#include "Precomp.h"
#include "Components/UtililtyAi/States/AttackingState.h"

#include "Utilities/AiFunctionality.h"
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

void Game::AttackingState::OnAiTick(CE::World& world, const entt::entity owner, float) const
{
	AIFunctionality::FaceThePlayer(world, owner);

	AIFunctionality::ExecuteEnemyAbility(world, owner);

	auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);

	if (physicsBody2DComponent == nullptr)
	{
		LOG(LogAI, Warning, "Attacking State - enemy {} does not have a PhysicsBody2D Component.", entt::to_integral(owner));
		return;
	}

	physicsBody2DComponent->mLinearVelocity = { 0,0 };
}

float Game::AttackingState::OnAiEvaluate(const CE::World& world, const entt::entity owner) const
{
	const auto score = AIFunctionality::GetBestScoreBasedOnDetection(world, owner, mRadius);
	return score;
}

void Game::AttackingState::OnAiStateEnter(CE::World& world, entt::entity owner)
{
	AIFunctionality::AnimationInAi(world, owner, mAttackingAnimation, false);
}

CE::MetaType Game::AttackingState::Reflect()
{
	auto type = CE::MetaType{CE::MetaType::T<AttackingState>{}, "AttackingState"};
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddField(&AttackingState::mRadius, "Detection Radius").GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &AttackingState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &AttackingState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &AttackingState::OnAiStateEnter);

	type.AddField(&AttackingState::mAttackingAnimation, "Attacking Animation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<AttackingState>(type);
	return type;
}
