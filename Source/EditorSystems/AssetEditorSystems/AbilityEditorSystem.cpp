#include "Precomp.h"
#include "EditorSystems/AssetEditorSystems/AbilityEditorSystem.h"

#include "Utilities/Imgui/ImguiInspect.h"
#include "Meta/MetaManager.h"

CE::AbilityEditorSystem::AbilityEditorSystem(Ability&& asset)
	: AssetEditorSystem(std::move(asset))
{
}

void CE::AbilityEditorSystem::Tick(float deltaTime)
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

	const MetaType& ability = MetaManager::Get().GetType<Ability>();

	MetaAny refToMat{ mAsset };

	for (const MetaField& field : ability.EachField())
	{
		MetaAny refToMember = field.MakeRef(refToMat);
		ShowInspectUI(std::string{ field.GetName() }, refToMember);
	}
	End();
}

CE::MetaType CE::AbilityEditorSystem::Reflect()
{
	return { MetaType::T<AbilityEditorSystem>{}, "AbilityEditorSystem",
		MetaType::Base<AssetEditorSystem<Ability>>{},
		MetaType::Ctor<Ability&&>{} };
}
