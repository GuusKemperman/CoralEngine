#include "Precomp.h"
#include "Components/CorpseComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void Game::CorpseComponent::OnBeginPlay(const CE::World& world, entt::entity)
{
	mTimeOfDeath = world.GetCurrentTimeScaled();
}

CE::MetaType Game::CorpseComponent::Reflect()
{
	CE::MetaType type{ CE::MetaType::T<CorpseComponent>{}, "CorpseComponent" };
	CE::MetaProps& props = type.GetProperties();
	props.Add(CE::Props::sIsScriptableTag);

	CE::BindEvent(type, CE::sBeginPlayEvent, &CorpseComponent::OnBeginPlay);

	type.AddField(&CorpseComponent::mTimeOfDeath, "mTimeOfDeath").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<CorpseComponent>(type);
	return type;
}
