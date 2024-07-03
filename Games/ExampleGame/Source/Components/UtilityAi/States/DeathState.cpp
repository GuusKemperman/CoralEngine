#include "Precomp.h"
#include "Components/UtililtyAi/States/DeathState.h"

#include <entt/entity/runtime_view.hpp>

#include "Components/Abilities/CharacterComponent.h"
#include "Meta/MetaType.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Components/AnimationRootComponent.h"
#include "Components/Physics2D/DiskColliderComponent.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Assets/Animation/Animation.h"
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
#include "Components/ScoreComponent.h"

void Game::DeathState::OnTick(CE::World& world, const entt::entity owner, const float dt)
{
	if (!mHasStateBeenEntered)
	{
		return;
	}

	auto& registry = world.GetRegistry();

	// Dim eye lights.

	if (mCurrentDeathTimer < mLightFadeOutDuration)
	{
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
		removeLights(removeLights, *transform, std::max((mLightFadeOutDuration - mCurrentDeathTimer + dt) * 0.5f, 0.0f));
	}

	mCurrentDeathTimer += dt;
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

void Game::DeathState::OnAiStateEnterEvent(CE::World& world, entt::entity owner)
{
	auto& registry = world.GetRegistry();
	// Corpses shouldn't think
	registry.RemoveComponent<CE::EnemyAiControllerComponent>(owner);
	mHasStateBeenEntered = true;

	// Call On Enemy Killed events.
	const auto playerView = registry.View<CE::PlayerComponent>();
	if (!playerView.empty())
	{
		const auto player = playerView.front();
		auto& boundEvents = CE::AbilitySystem::GetEnemyKilledEvents();
		for (const CE::BoundEvent& boundEvent : boundEvents)
		{
			entt::sparse_set* const storage = registry.Storage(boundEvent.mType.get().GetTypeId());

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

	auto* animationRootComponent = registry.TryGet<CE::AnimationRootComponent>(owner);

	if (animationRootComponent != nullptr)
	{
		const float time = mDeathAnimation == nullptr ? 0.0f : mDeathAnimation->mDuration * mAnimationStartTimePercentage;
		animationRootComponent->SwitchAnimation(registry, mDeathAnimation, time, mAnimationSpeed);
	}
	else
	{
		LOG(LogAI, Warning, "Enemy {} does not have a AnimationRoot Component.", entt::to_integral(owner));
	}

	auto* physicsBody2DComponent = registry.TryGet<CE::PhysicsBody2DComponent>(owner);
	if (physicsBody2DComponent != nullptr)
	{
		physicsBody2DComponent->mLinearVelocity = { 0,0 };
	}

	CE::SwarmingAgentTag::StopMovingToTarget(world, owner);

	registry.RemoveComponentIfEntityHasIt<CE::PhysicsBody2DComponent>(owner);
	registry.RemoveComponentIfEntityHasIt<CE::DiskColliderComponent>(owner);

	auto* transform = registry.TryGet<CE::TransformComponent>(owner);
	if (transform == nullptr)
	{
		LOG(LogAbilitySystem, Error, "Character with entity id {} does not have a TransformComponent attached.", entt::to_integral(owner));
	}
	else
	{
		const auto setMeshColor = [&registry](const auto& self, const CE::TransformComponent& current) -> void
			{
				for (const CE::TransformComponent& child : current.GetChildren())
				{
					auto* staticMesh = registry.TryGet<CE::StaticMeshComponent>(child.GetOwner());
					if (staticMesh != nullptr)
					{
						staticMesh->mHighlightedMesh = false;
					}

					auto* skinnedMesh = registry.TryGet<CE::SkinnedMeshComponent>(child.GetOwner());
					if (skinnedMesh != nullptr)
					{
						skinnedMesh->mHighlightedMesh = false;
					}

					self(self, child);
				}
			};
		setMeshColor(setMeshColor, *transform);

		// Checking for leveling script because if the player is not leveling anymore, XP shouldn't be spawned.
		const CE::MetaType* levellingScript = CE::MetaManager::Get().TryGetType("S_LevellingScript");
		if (levellingScript == nullptr)
		{
			LOG(LogGame, Error, "Could not find S_LevellingScript.");
		}
		else
		{
			entt::sparse_set* levellingStorage = registry.Storage(levellingScript->GetTypeId());
			if (levellingStorage != nullptr)
			{
				entt::runtime_view levellingView{};
				levellingView.iterate(*levellingStorage);
				if (mExpOrb != nullptr && levellingView.begin() != levellingView.end())
				{
					registry.CreateFromPrefab(*mExpOrb, entt::null, nullptr, nullptr, nullptr, transform);
				}
			}
		}
	}

	const auto scoreComponent = registry.TryGet<Game::ScoreComponent>(world.GetRegistry().View<Game::ScoreComponent>().front());

	if (scoreComponent != nullptr)
	{
		scoreComponent->mTotalScore += 1;
	}
	else
	{
		LOG(LogAI, Warning, "There is no score component in this ***REMOVED***ne!");
	}
}

void Game::DeathState::OnFinishAnimationEvent(CE::World& world, entt::entity owner)
{
	if (!mHasStateBeenEntered)
	{
		return;
	}

	world.GetRegistry().Destroy(owner, true);
}

CE::MetaType Game::DeathState::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<DeathState>{}, "DeathState" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sTickEvent, &DeathState::OnTick);
	BindEvent(type, CE::sAIEvaluateEvent, &DeathState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &DeathState::OnAiStateEnterEvent);
	BindEvent(type, CE::sAnimationFinishEvent, &DeathState::OnFinishAnimationEvent);

	type.AddField(&DeathState::mDeathAnimation, "Death Animation").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DeathState::mLightFadeOutDuration, "Light Fade Out Duration").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DeathState::mAnimationStartTimePercentage, "Animation Start Time Percentage").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DeathState::mAnimationSpeed, "Animation Speed").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DeathState::mExpOrb, "Exp Orb Spawner Prefab").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<DeathState>(type);
	return type;
}
