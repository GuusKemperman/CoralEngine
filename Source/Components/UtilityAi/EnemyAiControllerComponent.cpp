#include "Precomp.h"
#include "Components/UtilityAi/EnemyAiControllerComponent.h"

#include "Components/ComponentFilter.h"
#include "Utilities/Reflect/ReflectComponentType.h"

#ifdef EDITOR
#include "Utilities/Imgui/ImguiInspect.h"

void CE::EnemyAiControllerComponent::OnInspect(World& world, const std::vector<entt::entity>& entities)
{
	if (entities.size() > 1)
	{
		ImGui::TextUnformatted("Cannot inspect more than one AI controller at a time");
		return;
	}

	const entt::entity entity = entities.front();
	const EnemyAiControllerComponent* const aiController = world.GetRegistry().TryGet<EnemyAiControllerComponent>(entity);

	if (aiController == nullptr)
	{
		LOG(LogEditor, Error, "AIController was unexpectedly nullptr");
		return;
	}

	ImGui::Text("Current state: %s", aiController->mCurrentState == nullptr ? "None" : aiController->mCurrentState->GetName().c_str());

	if (ImGui::BeginTable("table1", 2))
	{
		ImGui::TableSetupColumn("State");
		ImGui::TableSetupColumn("Score");
		ImGui::TableHeadersRow();

		for (const auto& [name, score] : aiController->mDebugPreviouslyEvaluatedScores)
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted(name.data());
			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%f", score);
		}

		ImGui::EndTable();
	}
}
#endif // EDITOR

CE::MetaType CE::EnemyAiControllerComponent::Reflect()
{
	auto type = MetaType{MetaType::T<EnemyAiControllerComponent>{}, "EnemyAIControllerComponent"};
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);

#ifdef EDITOR
	BindEvent(type, sInspectEvent, &EnemyAiControllerComponent::OnInspect);
#endif // EDITOR

	type.AddFunc([](const EnemyAiControllerComponent& enemyAiController, const ComponentFilter& component) -> bool
		{
			return enemyAiController.mCurrentState == component;

		}, "IsInState", MetaFunc::ExplicitParams<const EnemyAiControllerComponent&, const ComponentFilter&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

	ReflectComponentType<EnemyAiControllerComponent>(type);
	return type;
}
