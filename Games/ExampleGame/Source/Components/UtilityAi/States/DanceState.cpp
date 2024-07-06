#include "Precomp.h"
#include "Components/UtililtyAi/States/DanceState.h"

#include "Utilities/AiFunctionality.h"
#include "Components/Abilities/CharacterComponent.h"
#include "Meta/MetaType.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Assets/Animation/Animation.h"
#include "Components/AnimationRootComponent.h"
#include "Components/PlayerComponent.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"

void Game::DanceState::OnAiTick(CE::World& world, const entt::entity owner, const float)
{
	auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);

	if (physicsBody2DComponent == nullptr)
	{
		LOG(LogAI, Warning, "Dance State - enemy {} does not have a PhysicsBody2D Component.", entt::to_integral(owner));
		return;
	}

	physicsBody2DComponent->mLinearVelocity = { 0,0 };
}

float Game::DanceState::OnAiEvaluate(const CE::World& world, [[maybe_unused]] const entt::entity owner)
{
	const entt::entity playerId = world.GetRegistry().View<CE::PlayerComponent>().front();

	if (playerId == entt::null)
	{
		return 1.0f;
	}

	const auto characterComponent = world.GetRegistry().TryGet<CE::CharacterComponent>(playerId);

	if (characterComponent == nullptr
		|| characterComponent->mCurrentHealth <= 0.f)
	{
		return 1.0f;
	}

	return 0.f;
}

void Game::DanceState::OnAiStateEnterEvent(CE::World& world, const entt::entity owner) const
{
	AIFunctionality::AnimationInAi(world, owner, mDanceAnimation, true);
}

CE::MetaType Game::DanceState::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<DanceState>{}, "DanceState" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &DanceState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &DanceState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &DanceState::OnAiStateEnterEvent);

	type.AddField(&DanceState::mDanceAnimation, "Dance Animation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<DanceState>(type);
	return type;
}
