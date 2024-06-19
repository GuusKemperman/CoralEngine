#include "Precomp.h"
#include "Components/CameraShakeDown.h"

#include "Components/TransformComponent.h"
#include "Meta/MetaType.h"
#include "Utilities/DrawDebugHelpers.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Components/AnimationRootComponent.h"
#include "Components/CameraComponent.h"
#include "Components/Abilities/AbilitiesOnCharacterComponent.h"
#include "Systems/AbilitySystem.h"

void Game::CameraShakeDown::OnTick(CE::World& world, const entt::entity owner, const float dt)
{
	static const siv::BasicPerlinNoise<float> noise{ 12345 };

	{
		auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

		if (transformComponent == nullptr)
		{
			LOG(LogAI, Warning, "Chasing State - enemy {} does not have a Transform Component.", entt::to_integral(owner));
			return;
		}

		float offsetX = std::min(mCurrentIntensity, 1.f)* (mRange * noise.noise1D(mShakeSpeed * world.GetCurrentTimeScaled()));
		float offsetY = std::min(mCurrentIntensity, 1.f)* (mRange * noise.noise1D(mShakeSpeed * world.GetCurrentTimeScaled() + 456188.f));

		transformComponent->SetLocalPosition({ offsetX, offsetY, 0 });

		mCurrentIntensity = std::max(mCurrentIntensity - dt * mFallOffSpeed, 0.0f);
	}
}

void Game::CameraShakeDown::AddShake(float intensity)
{
	mCurrentIntensity = intensity;
}

CE::MetaType Game::CameraShakeDown::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<CameraShakeDown>{}, "CameraShakeDown" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sTickEvent, &CameraShakeDown::OnTick);

	type.AddField(&CameraShakeDown::mRange, "Range").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&CameraShakeDown::mShakeSpeed, "Speed").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&CameraShakeDown::mFallOffSpeed, "Fall Off Speed").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&CameraShakeDown::mCurrentIntensity, "CurrentIntensity").GetProperties().Add(CE::Props::sIsEditorReadOnlyTag);

	type.AddFunc([](const float intensity)
		{
			CE::World* world = CE::World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			const auto cameraShake = world->GetRegistry().TryGet<CameraShakeDown>(world->GetRegistry().View<CE::CameraComponent>().front());

			if (cameraShake == nullptr)
			{
				LOG(LogAI, Warning, "AddShake - Camera Component is missing!.");
				return;
			}

			cameraShake->AddShake(intensity);

		}, "AddShake", CE::MetaFunc::ExplicitParams< float>{}, "Intensity").GetProperties().Add(CE::Props::sIsScriptableTag).Set(CE::Props::sIsScriptPure, false);

	CE::ReflectComponentType<CameraShakeDown>(type);
	return type;
}
