#include "Precomp.h"
#include "Components/Abilities/AOEComponent.h"

#include "Meta/MetaType.h"
#include "Utilities/Reflect/ReflectComponentType.h"

Engine::MetaType Engine::AOEComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<AOEComponent>{}, "AOEComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	metaType.AddField(&AOEComponent::mDuration, "mDuration").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&AOEComponent::mCurrentDuration, "mCurrentDuration").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoInspectTag);

#ifdef EDITOR
	BindEvent(metaType, sInspectEvent, &AOEComponent::OnInspect);
#endif // EDITOR

	ReflectComponentType<AOEComponent>(metaType);

	return metaType;
}

#ifdef EDITOR
void Engine::AOEComponent::OnInspect([[maybe_unused]] World& world, [[maybe_unused]] const std::vector<entt::entity>& entities)
{
	ImGui::TextDisabled("Radius");
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::Text("If you want to set the radius:\n"
			"- For the collider, change the radius of the DiskColliderComponent\n"
			"- For the mesh, change the scale of the TransformComponent");
		ImGui::EndTooltip();
	}
}
#endif // EDITOR
