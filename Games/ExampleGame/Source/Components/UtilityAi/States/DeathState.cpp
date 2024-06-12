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
#include "Components/PlayerComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/TransformComponent.h"
#include "Components/Pathfinding/SwarmingAgentTag.h"
#include "Systems/AbilitySystem.h"

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
	const auto removeLights = [&registry, dt](const auto& self, const CE::TransformComponent& current, float timeLeft) -> void
		{
			for (const CE::TransformComponent& child : current.GetChildren())
			{
				auto light = registry.TryGet<CE::PointLightComponent>(child.GetOwner());
				if (light != nullptr)
				{
					const float decreaseAmount = light->mIntensity * (dt / timeLeft);
					light->mIntensity = std::max(light->mIntensity - decreaseAmount, 0.f);
				}
				self(self, child, timeLeft);
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

void Game::DeathState::OnAiStateEnterEvent(CE::World& world, entt::entity owner) const
{
	// Call On Enemy Killed events.
	auto playerView = world.GetRegistry().View<CE::PlayerComponent>();
	if (!playerView.empty())
	{
		const auto player = playerView.front();
		auto& boundEvents = CE::AbilitySystem::GetEnemyKilledEvents();
		for (const CE::BoundEvent& boundEvent : boundEvents)
		{
			entt::sparse_set* const storage = world.GetRegistry().Storage(boundEvent.mType.get().GetTypeId());

			if (storage == nullptr
				|| !storage->contains(player))
			{
				continue;
			}

			if (boundEvent.mIsStatic)
			{
				boundEvent.mFunc.get().InvokeUncheckedUnpacked(world, player);
			}
			else
			{
				CE::MetaAny component{ boundEvent.mType, storage->value(player), false };
				boundEvent.mFunc.get().InvokeUncheckedUnpacked(component, world, player, owner);
			}
		}
	}

	Game::AnimationInAi(world, owner, mDeathAnimation, false);

	auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);
	if (physicsBody2DComponent != nullptr)
	{
		physicsBody2DComponent->mLinearVelocity = { 0,0 };
	}

	CE::SwarmingAgentTag::StopMovingToTarget(world, owner);

	world.GetRegistry().RemoveComponentIfEntityHasIt<CE::PhysicsBody2DComponent>(owner);

	world.GetRegistry().RemoveComponentIfEntityHasIt<CE::DiskColliderComponent>(owner);
}

CE::MetaType Game::DeathState::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<DeathState>{}, "DeathState" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &DeathState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &DeathState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &DeathState::OnAiStateEnterEvent);

	type.AddField(&DeathState::mDeathAnimation, "Death Animation").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DeathState::mDestroyEntityWhenDead, "Destroy The Entity When Dead").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DeathState::mMaxDeathTime, "Max Death Time").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DeathState::mAnimationBlendTime, "Animation Blend Time").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<DeathState>(type);
	return type;
}
