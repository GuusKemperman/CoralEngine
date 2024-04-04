#include "Precomp.h"
#include "Components/Abilities/ProjectileComponent.h"

#include "Meta/MetaType.h"
#include "Utilities/Reflect/ReflectComponentType.h"

CE::MetaType CE::ProjectileComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<ProjectileComponent>{}, "ProjectileComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	metaType.AddField(&ProjectileComponent::mDestroyOnRangeReached, "mDestroyOnRangeReached").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&ProjectileComponent::mRange, "mRange").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&ProjectileComponent::mCurrentRange, "mCurrentRange").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsEditorReadOnlyTag);
	metaType.AddField(&ProjectileComponent::mSpeed, "mSpeed").GetProperties().Add(Props::sIsScriptableTag);

#ifdef EDITOR
	BindEvent(metaType, sInspectEvent, &ProjectileComponent::OnInspect);
#endif // EDITOR

	ReflectComponentType<ProjectileComponent>(metaType);

	return metaType;
}

#ifdef EDITOR
void CE::ProjectileComponent::OnInspect(World&, const std::vector<entt::entity>&)
{
	ImGui::TextDisabled("Destroy On Range Reached (?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::Text("This is set to true by default - the entity gets destroyed when it reaches the set range.\n"
			"However you may choose to disable this and make the ability get destroyed based on its lifetime\n"
			"by adding the AbilityLifeTimeComponent if it has not already been added.");
		ImGui::EndTooltip();
	}
}
#endif // EDITOR
