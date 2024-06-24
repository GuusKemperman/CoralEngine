#include "Precomp.h"
#include "Components/HighScoreTextComponent.h"

#include "Components/ScoreComponent.h"
#include "Components/TransformComponent.h"
#include "Components/UI/UITextComponent.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "World/World.h"
#include "World/Registry.h"

void Game::HighScoreTextComponent::OnTick(CE::World& world, const entt::entity owner, float)
{
	auto* uiTextComponent = world.GetRegistry().TryGet<CE::UITextComponent>(owner);

	if (uiTextComponent == nullptr)
	{
		LOG(LogAI, Warning, "A UIText component is needed to run the High Score Text Component!");
		return;
	}

	const auto scoreComponent = world.GetRegistry().TryGet<Game::ScoreComponent>(world.GetRegistry().View<Game::ScoreComponent>().front());

	if (scoreComponent == nullptr)
	{
		LOG(LogAI, Warning, "There is no score component in this ***REMOVED***ne!");
		return;
	}

	uiTextComponent->mText = "HighScore: " + std::to_string(scoreComponent->CheckHighScore());

	auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

	if (transformComponent == nullptr)
	{
		LOG(LogAI, Warning, "A Transform component is needed to run the High Score Text Component!");
		return;
	}

	transformComponent->SetLocalPosition({transformComponent->GetLocalPosition().x, transformComponent->GetLocalPosition().y, 0});
}

CE::MetaType Game::HighScoreTextComponent::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<HighScoreTextComponent>{}, "HighScoreTextComponent" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sTickEvent, &HighScoreTextComponent::OnTick);

	CE::ReflectComponentType<HighScoreTextComponent>(type);
	return type;
}
