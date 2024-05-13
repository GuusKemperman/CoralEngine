#include "Precomp.h"
#include "EditorSystems/AssetEditorSystems/WeaponEditorSystem.h"

#include "Utilities/Imgui/ImguiInspect.h"
#include "Meta/MetaManager.h"

CE::WeaponEditorSystem::WeaponEditorSystem(Weapon&& asset)
	: AssetEditorSystem(std::move(asset))
{
}

void CE::WeaponEditorSystem::Tick(float deltaTime)
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

	const MetaType& weapon = MetaManager::Get().GetType<Weapon>();

	MetaAny refToMat{ mAsset };

	for (const MetaField& field : weapon.EachField())
	{
		MetaAny refToMember = field.MakeRef(refToMat);
		ShowInspectUI(std::string{ field.GetName() }, refToMember);
	}
	End();
}

CE::MetaType CE::WeaponEditorSystem::Reflect()
{
	return { MetaType::T<WeaponEditorSystem>{}, "WeaponEditorSystem",
		MetaType::Base<AssetEditorSystem<Weapon>>{},
		MetaType::Ctor<Weapon&&>{} };
}

