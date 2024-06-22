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
#include "Components/AttachToBoneComponent.h"
#include "Components/MeshColorComponent.h"
#include "Components/PlayerComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TransformComponent.h"
#include "Components/Pathfinding/SwarmingAgentTag.h"
#include "Components/UtilityAi/EnemyAiControllerComponent.h"
#include "Systems/AbilitySystem.h"
#include "Utilities/DrawDebugHelpers.h"
#include "Components/Abilities/AbilitiesOnCharacterComponent.h"
#include "Assets/Prefabs/Prefab.h"

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
					light->mIntensity = light->mIntensity - decreaseAmount;

					if (light->mIntensity <= 0.0f)
					{
						registry.RemoveComponent<CE::PointLightComponent>(child.GetOwner());
					}
				}
				self(self, child, timeLeft);
			}
		};
	removeLights(removeLights, *transform, std::max((mLightFadeOutDuration - mCurrentDeathTimer) * 0.5f, 0.0f));
	mCurrentDeathTimer += dt;

	if (mSink
		&& mCurrentDeathTimer > mStartSinkingDelay) 
	{
		glm::vec3 positionChange = transform->GetWorldPosition();
		positionChange.y -= mSinkDownSpeed;

		// Update death timer.
		if (positionChange.y < mDestroyWhenBelowHeight)
		{
			registry.Destroy(owner, true);
			return;
		}

		transform->SetWorldPosition(positionChange);
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
	const auto playerView = world.GetRegistry().View<CE::PlayerComponent>();
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

	auto* animationRootComponent = world.GetRegistry().TryGet<CE::AnimationRootComponent>(owner);

	if (animationRootComponent != nullptr)
	{
		const float time = mDeathAnimation == nullptr ? 0.0f : mDeathAnimation->mDuration * mAnimationStartTimePercentage;
		animationRootComponent->SwitchAnimation(world.GetRegistry(), mDeathAnimation, time, mAnimationSpeed);
	}
	else
	{
		LOG(LogAI, Warning, "Enemy {} does not have a AnimationRoot Component.", entt::to_integral(owner));
	}

	auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);
	if (physicsBody2DComponent != nullptr)
	{
		physicsBody2DComponent->mLinearVelocity = { 0,0 };
	}

	CE::SwarmingAgentTag::StopMovingToTarget(world, owner);

	world.GetRegistry().RemoveComponentIfEntityHasIt<CE::PhysicsBody2DComponent>(owner);

	world.GetRegistry().RemoveComponentIfEntityHasIt<CE::DiskColliderComponent>(owner);

	auto* transform = world.GetRegistry().TryGet<CE::TransformComponent>(owner);
	if (transform == nullptr)
	{
		LOG(LogAbilitySystem, Error, "Character with entity id {} does not have a TransformComponent attached.", entt::to_integral(owner));
	}
	else
	{
		const auto setMeshColor = [&world](const auto& self, const CE::TransformComponent& current) -> void
			{
				for (const CE::TransformComponent& child : current.GetChildren())
				{
					auto* staticMesh = world.GetRegistry().TryGet<CE::StaticMeshComponent>(child.GetOwner());
					if (staticMesh != nullptr)
					{
						staticMesh->mHighlightedMesh = false;
					}

					auto* skinnedMesh = world.GetRegistry().TryGet<CE::SkinnedMeshComponent>(child.GetOwner());
					if (skinnedMesh != nullptr)
					{
						skinnedMesh->mHighlightedMesh = false;
					}

					self(self, child);
				}
			};
		setMeshColor(setMeshColor, *transform);
	}

	if (transform == nullptr)
	{
		LOG(LogAI, Warning, "Death State - enemy {} does not have a Transform Component.", entt::to_integral(owner));
		return;
	}

	if (mExpOrb != nullptr) {
		world.GetRegistry().CreateFromPrefab(*mExpOrb, entt::null, nullptr, nullptr, nullptr, transform);
	}
}

void Game::DeathState::OnFinishAnimationEvent(CE::World& world, entt::entity owner)
{
	const auto* enemyAiController = world.GetRegistry().TryGet<CE::EnemyAiControllerComponent>(owner);

	if(CE::MakeTypeId<DeathState>() == enemyAiController->mCurrentState->GetTypeId())
	{
		auto* animationRootComponent = world.GetRegistry().TryGet<CE::AnimationRootComponent>(owner);

		if (animationRootComponent != nullptr)
		{
			animationRootComponent->SwitchAnimation(world.GetRegistry(), mDeathAnimation, animationRootComponent->mCurrentAnimation->mDuration -1.0f, 0, 0.0f);
		}
		else
		{
			LOG(LogAI, Warning, "Enemy {} does not have a AnimationRoot Component.", entt::to_integral(owner));
		}

		mSink = true;
	}
}

CE::MetaType Game::DeathState::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<DeathState>{}, "DeathState" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &DeathState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &DeathState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &DeathState::OnAiStateEnterEvent);
	BindEvent(type, CE::sAnimationFinishEvent, &DeathState::OnFinishAnimationEvent);

	type.AddField(&DeathState::mDeathAnimation, "Death Animation").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DeathState::mDestroyEntityWhenDead, "Destroy The Entity When Dead").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DeathState::mStartSinkingDelay, "Start Sinking Delay").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DeathState::mLightFadeOutDuration, "Light Fade Out Duration").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DeathState::mAnimationStartTimePercentage, "Animation Start Time Percentage").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DeathState::mAnimationSpeed, "Animation Speed").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DeathState::mSinkDownSpeed, "Sink Down Speed").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DeathState::mDestroyWhenBelowHeight, "Destroy When Below Height").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DeathState::mExpOrb, "Exp Orb Spawner Prefab").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<DeathState>(type);
	return type;
}
