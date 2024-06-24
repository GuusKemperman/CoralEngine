#include "Precomp.h"
#include "Components/ScoreTextComponent.h"

#include "Components/ScoreComponent.h"
#include "Components/TransformComponent.h"
#include "Components/UI/UITextComponent.h"
#include "World/Registry.h"
#include "World/World.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void Game::ScoreTextComponent::OnTick(CE::World& world, const entt::entity owner, float) const
{
	auto* uiTextComponent = world.GetRegistry().TryGet<CE::UITextComponent>(owner);

	if (uiTextComponent == nullptr)
	{
		LOG(LogAI, Warning, "A UIText component is needed to run the Score Text Component!");
		return;
	}

	const auto scoreComponent = world.GetRegistry().TryGet<Game::ScoreComponent>(world.GetRegistry().View<Game::ScoreComponent>().front());

	if (scoreComponent == nullptr)
	{
		LOG(LogAI, Warning, "There is no score component in this ***REMOVED***ne!");
		return;
	}

	if (mDisplayHighScore) {
		uiTextComponent->mText = std::to_string(scoreComponent->CheckHighScore());
	}
	else
	{
		uiTextComponent->mText = std::to_string(scoreComponent->mTotalScore);
	}
}

CE::MetaType Game::ScoreTextComponent::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<ScoreTextComponent>{}, "ScoreTextComponent" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sTickEvent, &ScoreTextComponent::OnTick);

	type.AddField(&ScoreTextComponent::mDisplayHighScore, "Display High Score").GetProperties().Add(CE::Props::sIsScriptableTag);


	CE::ReflectComponentType<ScoreTextComponent>(type);
	return type;
}
