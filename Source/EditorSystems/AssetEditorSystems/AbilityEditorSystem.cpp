#include "Precomp.h"
#include "EditorSystems/AssetEditorSystems/AbilityEditorSystem.h"

#include "Utilities/Imgui/ImguiInspect.h"

Engine::AbilityEditorSystem::AbilityEditorSystem(Ability&& asset)
	: AssetEditorSystem(std::move(asset))
{
}

void Engine::AbilityEditorSystem::Tick(float deltaTime)
{
	if (!Begin(ImGuiWindowFlags_MenuBar))
	{
		End();
		return;
	}

	AssetEditorSystem::Tick(deltaTime);

	if (ImGui::BeginMenuBar())
	{
		ShowSaveButton();
		ImGui::EndMenuBar();
	}

	const MetaType* const ability = MetaManager::Get().TryGetType<Ability>();
	ASSERT(ability != nullptr);

	MetaAny refToMat{ mAsset };

	for (const MetaField& field : ability->EachField())
	{
		MetaAny refToMember = field.MakeRef(refToMat);
		ShowInspectUI(std::string{ field.GetName() }, refToMember);
	}
	End();
}

Engine::MetaType Engine::AbilityEditorSystem::Reflect()
{
	return { MetaType::T<AbilityEditorSystem>{}, "AbilityEditorSystem",
		MetaType::Base<AssetEditorSystem<Ability>>{},
		MetaType::Ctor<Ability&&>{} };
}
