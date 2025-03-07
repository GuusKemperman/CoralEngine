#include "Precomp.h"
#include "Components/CameraShakeComponent.h"

#include "Components/TransformComponent.h"
#include "Meta/MetaType.h"
#include "Utilities/DrawDebugHelpers.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Components/AnimationRootComponent.h"
#include "Components/CameraComponent.h"
#include "Components/Abilities/AbilitiesOnCharacterComponent.h"
#include "Systems/AbilitySystem.h"
#include "Utilities/Random.h"

void Game::CameraShakeComponent::OnTick(CE::World& world, const entt::entity owner, const float dt)
{
	float offsetX{};
	float offsetY{};

	for (auto effect = mActiveEffects.begin(); effect != mActiveEffects.end(); )
	{
		siv::BasicPerlinNoise<float> noise{ static_cast<siv::BasicPerlinNoise<float>::seed_type>(world.GetCurrentTimeScaled()) };

		offsetX += std::min(effect->mTimeLeft, mFadeOutAtIntensity) * (effect->mRangeX * noise.noise1D(effect->mShakeSpeed * world.GetCurrentTimeScaled()));
		offsetY += std::min(effect->mTimeLeft, mFadeOutAtIntensity) * (effect->mRangeY * noise.noise1D(effect->mShakeSpeed * world.GetCurrentTimeScaled() + 456188.f));


		effect->mTimeLeft -= dt;
		if (effect->mTimeLeft < 0)
		{
			effect = mActiveEffects.erase(effect);
		}
		else
		{
			++effect;
		}
	}

	auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

	if (transformComponent == nullptr)
	{
		LOG(LogAI, Warning, "AddShake - camera {} does not have a Transform Component.", entt::to_integral(owner));
		return;
	}

	transformComponent->SetLocalPosition({ offsetX, offsetY, 0 });
}

void Game::CameraShakeComponent::AddShake(CE::World& world, const float rangeX, const float rangeY, const float duration, const float speed)
{
	const auto cameraShake = world.GetRegistry().TryGet<CameraShakeComponent>(world.GetRegistry().View<CE::CameraComponent>().front());

	if (cameraShake == nullptr)
	{
		LOG(LogAI, Warning, "AddShake - Shake Component Component is nullptr!.");
		return;
	}

	ShakeEffect shakeEffect = {duration, speed, rangeX, rangeY};

	cameraShake->mActiveEffects.emplace_back(shakeEffect);
}

CE::MetaType Game::CameraShakeComponent::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<CameraShakeComponent>{}, "CameraShakeComponent" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sOnTick, &CameraShakeComponent::OnTick);

	type.AddField(&CameraShakeComponent::mFadeOutAtIntensity, "Fade Out Intensity").GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddFunc([](const float rangeX, const float rangeY, const float duration, const float speed)
		{
			CE::World* world = CE::World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			CameraShakeComponent::AddShake(*world, rangeX, rangeY, duration, speed);
		}, "AddShake", CE::MetaFunc::ExplicitParams< 
		float, float, float, float>{}, "RangeX", "RangeY", "Duration", "Speed").GetProperties().Add(CE::Props::sIsScriptableTag).Set(CE::Props::sIsScriptPure, false);

	CE::ReflectComponentType<CameraShakeComponent>(type);
	return type;
}
