#include "Precomp.h"
#include "Components/UtililtyAi/States/IdleState.h"

#include "Utilities/AiFunctionality.h"
#include "Meta/MetaType.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Assets/Animation/Animation.h"
#include "Components/AnimationRootComponent.h"
#include "Components/Pathfinding/SwarmingAgentTag.h"

void Game::IdleState::OnAiTick(CE::World& world, const entt::entity owner, float) const
{
	Game::AnimationInAi(world, owner, mIdleAnimation);
}

float Game::IdleState::OnAiEvaluate(const CE::World&, entt::entity)
{
	return 0.01f;
}

void Game::IdleState::OnAIStateEnterEvent(CE::World& world, const entt::entity owner)
{
	CE::SwarmingAgentTag::StopMovingToTarget(world, owner);
}

CE::MetaType Game::IdleState::Reflect()
{
	auto type = CE::MetaType{CE::MetaType::T<IdleState>{}, "IdleState"};
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &IdleState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &IdleState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &IdleState::OnAIStateEnterEvent);

	type.AddField(&IdleState::mIdleAnimation, "mIdleAnimation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<IdleState>(type);
	return type;
}
