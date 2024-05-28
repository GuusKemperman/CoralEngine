#include "Precomp.h"
#include "EditorSystems/AssetEditorSystems/UpgradeEditorSystem.h"

#include "Utilities/Imgui/ImguiInspect.h"
#include "Meta/MetaManager.h"

CE::UpgradeEditorSystem::UpgradeEditorSystem(Upgrade&& asset)
	: AssetEditorSystem(std::move(asset))
{
}

void CE::UpgradeEditorSystem::Tick(float deltaTime)
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

	const MetaType& upgrade = MetaManager::Get().GetType<Upgrade>();

	MetaAny refToMat{ mAsset };

	for (const MetaField& field : upgrade.EachField())
	{
		MetaAny refToMember = field.MakeRef(refToMat);
		ShowInspectUI(std::string{ field.GetName() }, refToMember);
	}
	End();
}

CE::MetaType CE::UpgradeEditorSystem::Reflect()
{
	return { MetaType::T<UpgradeEditorSystem>{}, "UpgradeEditorSystem",
		MetaType::Base<AssetEditorSystem<Upgrade>>{},
		MetaType::Ctor<Upgrade&&>{} };
}
