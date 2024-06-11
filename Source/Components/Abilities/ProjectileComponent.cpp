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
	metaType.AddField(&ProjectileComponent::mPierceCount, "mPierceCount").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&ProjectileComponent::mCurrentPierceCount, "mCurrentPierceCount").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsEditorReadOnlyTag);
	metaType.AddField(&ProjectileComponent::mDirectionOffsetAngle, "mDirectionOffsetAngle").GetProperties().Add(Props::sIsScriptableTag);

#ifdef EDITOR
	BindEvent(metaType, sInspectEvent, &ProjectileComponent::OnInspect);
#endif // EDITOR

	ReflectComponentType<ProjectileComponent>(metaType);

	return metaType;
}

#ifdef EDITOR
void CE::ProjectileComponent::OnInspect(World&, const std::vector<entt::entity>&)
{
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.1f, 1.0f));
	ImGui::Text("Destroy On Range Reached (?)");
	ImGui::PopStyleColor();
	ImGui::SetItemTooltip("This is set to true by default - the entity gets destroyed when it reaches the set range.\n"
			"However, you may choose to disable this and make the ability get destroyed based on its lifetime\n"
		"by adding the AbilityLifeTimeComponent, if it has not already been added.");
}
#endif // EDITOR
