#include "Precomp.h"
#include "Components/ScoreComponent.h"

#include "Utilities/Reflect/ReflectComponentType.h"

void Game::ScoreComponent::OnTick(CE::World&, entt::entity, float)
{

	if (mTotalScore > mMaxTotalScore)
	{
		mTotalScore = mMaxTotalScore;
	}
}

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

	BindEvent(type, CE::sTickEvent, &ScoreComponent::OnTick);

	type.AddField(&ScoreComponent::mTotalScore, "Total Score").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&ScoreComponent::mMaxTotalScore, "Max Total Score").GetProperties().Add(CE::Props::sIsScriptableTag);
	//type.AddField(&ScoreComponent::mHighScore, "High Score").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<ScoreComponent>(type);
	return type;
}
