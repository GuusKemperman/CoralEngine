#include "Precomp.h"
#include "Systems/DifficultyScalingSystem.h"

#include "World/World.h"
#include "World/Registry.h"
#include "Components/DifficultyScalingComponent.h"
#include "Meta/MetaType.h"

void Game::DifficultyScalingSystem::Update(CE::World& world, float)
{
	const auto view = world.GetRegistry().View<DifficultyScalingComponent>();

	for (auto [entity, scalingComponent] : view.each())
	{
		float time = (world.GetCurrentTimeReal() - scalingComponent.mLoopsElapsed * scalingComponent.mScaleTime) / scalingComponent.mScaleTime;

		if (!scalingComponent.mIsRepeating)
		{
			time = glm::min(time, 1.0f);
		}
		else
		{
			if (time > 1.0f)
			{
				++scalingComponent.mLoopsElapsed;
				float minHealth = scalingComponent.mMinHealthMultiplier;
				float minDamage = scalingComponent.mMinDamageMultiplier;

				scalingComponent.mMinHealthMultiplier = scalingComponent.mMaxHealthMultiplier;
				scalingComponent.mMinDamageMultiplier = scalingComponent.mMaxDamageMultiplier;
				scalingComponent.mMaxHealthMultiplier += scalingComponent.mMaxHealthMultiplier - minHealth;
				scalingComponent.mMaxDamageMultiplier += scalingComponent.mMaxDamageMultiplier - minDamage;
			}

			time = glm::mod((world.GetCurrentTimeReal() / scalingComponent.mScaleTime), 1.0f);
		}

		scalingComponent.mCurrentHealthMultiplier = scalingComponent.mScaleHPOverTime.GetValueAt(time) * (scalingComponent.mMaxHealthMultiplier - scalingComponent.mMinHealthMultiplier) + scalingComponent.mMinHealthMultiplier;
		scalingComponent.mCurrentDamageMultiplier = scalingComponent.mScaleDamageOverTime.GetValueAt(time) * (scalingComponent.mMaxDamageMultiplier - scalingComponent.mMinDamageMultiplier) + scalingComponent.mMinDamageMultiplier;
	}
}

CE::MetaType Game::DifficultyScalingSystem::Reflect()
{
	return CE::MetaType{CE::MetaType::T<DifficultyScalingSystem>{}, "DifficultyScalingSystem", CE::MetaType::Base<System>{}};
}