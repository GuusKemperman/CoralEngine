#include "Precomp.h"
#include "Components/Abilities/ActiveAbilityComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Imgui/ImguiInspect.h"
#include "Utilities/Reflect/ReflectComponentType.h"

#ifdef EDITOR
void CE::ActiveAbilityComponent::OnInspect(World&, entt::entity)
{
	ShowInspectUIReadOnly("CastByCharacterTeamID", mCastByCharacterData.mTeamId);
}
#endif // EDITOR

CE::MetaType CE::ActiveAbilityComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<ActiveAbilityComponent>{}, "ActiveAbilityComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&ActiveAbilityComponent::mCastByEntity, "mCastByEntity").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoSerializeTag);
	metaType.AddField(&ActiveAbilityComponent::mCastByCharacterData, "mCastByCharacterData").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoInspectTag).Add(Props::sNoSerializeTag);
#ifdef EDITOR
	BindEvent(metaType, sOnInspect, &ActiveAbilityComponent::OnInspect);
#endif // EDITOR

	ReflectComponentType<ActiveAbilityComponent>(metaType);

	return metaType;
}
