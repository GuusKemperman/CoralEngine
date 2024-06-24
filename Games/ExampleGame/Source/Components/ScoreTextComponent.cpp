#include "Precomp.h"
#include "Components/ScoreTextComponent.h"

#include "Components/ScoreComponent.h"
#include "Components/TransformComponent.h"
#include "Components/UI/UITextComponent.h"
#include "World/Registry.h"
#include "World/World.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void Game::ScoreTextComponent::OnTick(CE::World& world, const entt::entity owner, float)
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

	uiTextComponent->mText = "Score: " + std::to_string(scoreComponent->mTotalScore);

	auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

	if (transformComponent == nullptr)
	{
		LOG(LogAI, Warning, "A Transform component is needed to run the Score Text Component!");
		return;
	}

	transformComponent->SetLocalPosition({ transformComponent->GetLocalPosition().x, transformComponent->GetLocalPosition().y, 0 });
}

CE::MetaType Game::ScoreTextComponent::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<ScoreTextComponent>{}, "ScoreTextComponent" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sTickEvent, &ScoreTextComponent::OnTick);

	CE::ReflectComponentType<ScoreTextComponent>(type);
	return type;
}
