#include "Precomp.h"
#include "Components/UtililtyAi/States/AttackingState.h"

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

void Game::AttackingState::OnAITick(CE::World& world, entt::entity owner, float)
{
	auto* animationRootComponent = world.GetRegistry().TryGet<CE::AnimationRootComponent>(owner);

	if (animationRootComponent != nullptr)
	{
		animationRootComponent->SwitchAnimation(world.GetRegistry(), mAttackingAnimation, 0.0f);
	}

	auto [score, targetEntity] = GetBestScoreAndTarget(world, owner);
	mTargetEntity = targetEntity;

	const auto characterData = world.GetRegistry().TryGet<CE::CharacterComponent>(owner);
	if (characterData == nullptr)
	{
		LOG(LogAI, Warning, "Attacking State - enemy {} does not have a Character Component.", entt::to_integral(owner));
		return;
	}

	const auto abilities = world.GetRegistry().TryGet<CE::AbilitiesOnCharacterComponent>(owner);
	if (abilities == nullptr)
	{
		LOG(LogAI, Warning, "Attacking State - enemy {} does not have a AbilitiesOnCharacter Component.", entt::to_integral(owner));
		return;
	}

	if (abilities->mAbilitiesToInput.empty())
	{
		return;
	}

	CE::AbilitySystem::ActivateAbility(world, owner, *characterData, abilities->mAbilitiesToInput[0]);

	auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);

	if (physicsBody2DComponent == nullptr)
	{
		LOG(LogAI, Warning, "Attacking State - enemy {} does not have a PhysicsBody2D Component.", entt::to_integral(owner));
		return;
	}

	physicsBody2DComponent->mLinearVelocity = { 0,0 };
}

float Game::AttackingState::OnAIEvaluate(const CE::World& world, entt::entity owner)
{
	auto [score, entity] = GetBestScoreAndTarget(world, owner);
	mTargetEntity = entity;
	return score;
}

std::pair<float, entt::entity> Game::AttackingState::GetBestScoreAndTarget(const CE::World& world, entt::entity owner) const
{

	entt::entity entityId = world.GetRegistry().View<CE::PlayerComponent>().front();

	if (entityId == entt::null)
	{
		return { 0.0f, entt::null };
	}

	const CE::TransformComponent* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

	if (transformComponent == nullptr)
	{
		LOG(LogAI, Warning, "Attacking State - enemy {} does not have a Transform Component.", entt::to_integral(owner));
		return { 0.0f, entt::null };
	}

	float highestScore = 0.0f;

	const CE::TransformComponent* targetComponent = world.GetRegistry().TryGet<CE::TransformComponent>(entityId);

	if (transformComponent == nullptr)
	{
		LOG(LogAI, Warning, "Attacking State - player {} does not have a Transform Component.", entt::to_integral(entityId));
		return { 0.0f, entt::null };
	}

	const float distance = glm::distance(transformComponent->GetWorldPosition(), targetComponent->GetWorldPosition());

	float score = 0.0f;

	if (distance < mRadius)
	{
		score = 1 / distance;
		score += 1 / mRadius;
	}

	if (highestScore < score)
	{
		highestScore = score;
	}

	return { highestScore, entityId };
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
