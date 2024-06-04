#include "Precomp.h"
#include "Components/UtililtyAi/States/RecoveryState.h"

#include "Utilities/AiFunctionality.h"
#include "Components/Abilities/AbilitiesOnCharacterComponent.h"
#include "Meta/MetaType.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Components/AnimationRootComponent.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Components/UtililtyAi/States/ChargeUpDashState.h"
#include "Assets/Animation/Animation.h"
#include "Components/PlayerComponent.h"
#include "Components/TransformComponent.h"

void Game::RecoveryState::OnAiTick(CE::World& world, const entt::entity owner, const float dt)
{
	Game::AnimationInAi(world, owner, mDashRechargeAnimation);
	
	auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);

	if (physicsBody2DComponent == nullptr)
	{
		LOG(LogAI, Warning, "Recovery State - enemy {} does not have a PhysicsBody2D Component.", entt::to_integral(owner));
		return;
	}

	physicsBody2DComponent->mLinearVelocity = {};

	mRechargeCooldown.mAmountOfTimePassed += dt;

	Game::FaceThePlayer(world, owner);
}

float Game::RecoveryState::OnAiEvaluate(const CE::World&, entt::entity) const
{
	if (mRechargeCooldown.mAmountOfTimePassed < mRechargeCooldown.mCooldown)
	{
		return 1.0f;
	}

	return 0;
}

void Game::RecoveryState::OnAiStateEnterEvent(CE::World&, entt::entity)
{
	mRechargeCooldown.mCooldown = mMaxRechargeTime;
	mRechargeCooldown.mAmountOfTimePassed = 0.0f;
}

void Game::RecoveryState::OnBeginPlayEvent(CE::World&, entt::entity)
{
	mRechargeCooldown.mAmountOfTimePassed = mRechargeCooldown.mCooldown;
}

CE::MetaType Game::RecoveryState::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<RecoveryState>{}, "RecoveryState" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddField(&RecoveryState::mMaxRechargeTime, "mMaxRechargeTime").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&RecoveryState::mRechargeCooldown, "mRechargeCooldown").GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &RecoveryState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &RecoveryState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &RecoveryState::OnAiStateEnterEvent);
	BindEvent(type, CE::sBeginPlayEvent, &RecoveryState::OnBeginPlayEvent);

	type.AddField(&RecoveryState::mDashRechargeAnimation, "mDashRechargeAnimation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<RecoveryState>(type);
	return type;
}
