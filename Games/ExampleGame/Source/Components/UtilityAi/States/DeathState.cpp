#include "Precomp.h"
#include "Components/UtililtyAi/States/DeathState.h"

#include "Components/Abilities/CharacterComponent.h"
#include "Meta/MetaType.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Components/AnimationRootComponent.h"
#include "Utilities/AiFunctionality.h"
#include "Components/Physics2D/DiskColliderComponent.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Assets/Animation/Animation.h"
#include "Components/PointLightComponent.h"
#include "Components/TransformComponent.h"
#include "Components/Pathfinding/SwarmingAgentTag.h"

void Game::DeathState::OnAiTick(CE::World& world, const entt::entity owner, const float dt)
{
	if (!mDestroyEntityWhenDead)
	{
		return;
	}

	auto& registry = world.GetRegistry();

	// Dim eye lights.
	auto* transform = registry.TryGet<CE::TransformComponent>(owner);
	if (transform == nullptr)
	{
		LOG(LogAI, Warning, "Death State - enemy {} does not have a Transform Component.", entt::to_integral(owner));
		return;
	}
	const auto removeLights = [&registry, dt](const auto& self, const CE::TransformComponent& current, float maxDeathTime) -> void
		{
			for (const CE::TransformComponent& child : current.GetChildren())
			{
				auto light = registry.TryGet<CE::PointLightComponent>(child.GetOwner());
				if (light != nullptr)
				{
					const float decreaseAmount = light->mIntensity * (dt / maxDeathTime);
					light->mIntensity = std::max(light->mIntensity - decreaseAmount, 0.f);
				}
				self(self, child, maxDeathTime);
			}
		};
	removeLights(removeLights, *transform, (mMaxDeathTime - mCurrentDeathTimer) * 0.5f);

	// Update death timer.
	mCurrentDeathTimer += dt;
	if (mCurrentDeathTimer >= mMaxDeathTime )
	{
		registry.Destroy(owner, true);
	}
}

float Game::DeathState::OnAiEvaluate(const CE::World& world, entt::entity owner)
{
	const auto characterComponent = world.GetRegistry().TryGet<CE::CharacterComponent>(owner);

	if (characterComponent == nullptr) {
		LOG(LogAI, Warning, "Death State - enemy {} does not have a Character Component.", entt::to_integral(owner));
	}

	if (characterComponent->mCurrentHealth <= 0.f)
	{
		return std::numeric_limits<float>::infinity();
	}

	return 0.f;
}

void Game::DeathState::OnAIStateEnterEvent(CE::World& world, entt::entity owner) const
{
	auto& registry = world.GetRegistry();
	auto* animationRootComponent = registry.TryGet<CE::AnimationRootComponent>(owner);

	if (animationRootComponent != nullptr)
	{
		animationRootComponent->SwitchAnimation(registry, mDeathAnimation, 0.0f, 1.0f, 1.5f);
	}

	auto* physicsBody2DComponent = registry.TryGet<CE::PhysicsBody2DComponent>(owner);

	if (physicsBody2DComponent != nullptr)
	{
		physicsBody2DComponent->mLinearVelocity = { 0,0 };
	}

	CE::SwarmingAgentTag::StopMovingToTarget(world, owner);

	registry.RemoveComponentIfEntityHasIt<CE::PhysicsBody2DComponent>(owner);

	registry.RemoveComponentIfEntityHasIt<CE::DiskColliderComponent>(owner);
}

CE::MetaType Game::DeathState::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<DeathState>{}, "DeathState" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &DeathState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &DeathState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &DeathState::OnAIStateEnterEvent);

	type.AddField(&DeathState::mDeathAnimation, "mDeathAnimation").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DeathState::mDestroyEntityWhenDead, "mDestroyEntityWhenDead").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DeathState::mMaxDeathTime, "mMaxDeathTime").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DeathState::mAnimationBlendTime, "mAnimationBlendTime").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<DeathState>(type);
	return type;
}
