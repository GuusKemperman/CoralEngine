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
		float time = (world.GetCurrentTimeReal() - scalingComponent.mLoopsElapsed * scalingComponent.mScaleLength) / scalingComponent.mScaleLength;

		if (!scalingComponent.mIsRepeating)
		{
			time = glm::min(time, 1.0f);
		}
		else
		{
			if (time > 1.0f)
			{
				++scalingComponent.mLoopsElapsed;
				float tempHp = scalingComponent.mMinHPMultiplier;
				float tempDmg = scalingComponent.mMinDamageMultiplier;

				scalingComponent.mMinHPMultiplier = scalingComponent.mMaxHPMultiplier;
				scalingComponent.mMinDamageMultiplier = scalingComponent.mMaxDamageMultiplier;
				scalingComponent.mMaxHPMultiplier += scalingComponent.mMaxHPMultiplier - tempHp;
				scalingComponent.mMaxDamageMultiplier += scalingComponent.mMaxDamageMultiplier - tempDmg;
			}

			time = glm::mod((world.GetCurrentTimeReal() / scalingComponent.mScaleLength), 1.0f);
		}

		scalingComponent.mCurrentHPMultiplier = scalingComponent.mScaleHPOverTime.GetValueAt(time) * (scalingComponent.mMaxHPMultiplier - scalingComponent.mMinHPMultiplier) + scalingComponent.mMinHPMultiplier;
		scalingComponent.mCurrentDamageMultiplier = scalingComponent.mScaleDamageOverTime.GetValueAt(time) * (scalingComponent.mMaxDamageMultiplier - scalingComponent.mMinDamageMultiplier) + scalingComponent.mMinDamageMultiplier;
	}
}

CE::MetaType Game::DifficultyScalingSystem::Reflect()
{
	return CE::MetaType{CE::MetaType::T<DifficultyScalingSystem>{}, "DifficultyScalingSystem", CE::MetaType::Base<System>{}};
}