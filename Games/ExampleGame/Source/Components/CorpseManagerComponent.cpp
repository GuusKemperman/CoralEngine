#include "Precomp.h"
#include "Components/CorpseManagerComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"

CE::MetaType Game::CorpseManagerComponent::Reflect()
{
	CE::MetaType type{ CE::MetaType::T<CorpseManagerComponent>{}, "CorpseManagerComponent" };
	CE::MetaProps& props = type.GetProperties();
	props.Add(CE::Props::sIsScriptableTag);

	type.AddField(&CorpseManagerComponent::mSinkSpeed, "mSinkSpeed").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&CorpseManagerComponent::mDestroyAtHeight, "mDestroyAtHeight").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&CorpseManagerComponent::mDestroyAllInstantlyIfOutsideOfRange, "mDestroyAllInstantlyIfOutsideOfRange").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&CorpseManagerComponent::mMaxTimeAlive, "mMaxTimeAlive").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&CorpseManagerComponent::mMaxAlive, "mMaxAlive").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<CorpseManagerComponent>(type);
	return type;
}
