#include "Precomp.h"
#include "Components/UtililtyAi/States/IdleState.h"

#include "Utilities/AiFunctionality.h"
#include "Meta/MetaType.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Assets/Animation/Animation.h"
#include "Components/AnimationRootComponent.h"
#include "Components/TransformComponent.h"
#include "Components/Abilities/CharacterComponent.h"
#include "Components/Pathfinding/SwarmingAgentTag.h"
#include "Components/UtilityAi/EnemyAiControllerComponent.h"

float Game::IdleState::OnAiEvaluate(const CE::World&, entt::entity)
{
	return 0.01f;
}

void Game::IdleState::OnAiStateEnterEvent(CE::World& world, entt::entity owner) const
{
	CE::SwarmingAgentTag::StopMovingToTarget(world, owner);
	AIFunctionality::AnimationInAi(world, owner, mIdleAnimation, true);
}


CE::MetaType Game::IdleState::Reflect()
{
	auto type = CE::MetaType{CE::MetaType::T<IdleState>{}, "IdleState"};
	type.GetProperties().Add(CE::Props::sIsScriptableTag);
	
	BindEvent(type, CE::sOnAIEvaluate, &IdleState::OnAiEvaluate);
	BindEvent(type, CE::sOnAIStateEnter, &IdleState::OnAiStateEnterEvent);

	type.AddField(&IdleState::mIdleAnimation, "Idle Animation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<IdleState>(type);
	return type;
}
