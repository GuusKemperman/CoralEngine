#include "Precomp.h"
#include "Components/ScoreComponent.h"

#include "Utilities/Reflect/ReflectComponentType.h"

int Game::ScoreComponent::CheckHighScore() const
{
	if (mHighScore < mTotalScore)
	{
		mHighScore = mTotalScore;
	}

	return mHighScore;
}

CE::MetaType Game::ScoreComponent::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<ScoreComponent>{}, "ScoreComponent" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddField(&ScoreComponent::mTotalScore, "Total Score").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<ScoreComponent>(type);
	return type;
}
