#include "Precomp.h"
#include "EditorSystems/AssetEditorSystems/MaterialEditorSystem.h"

#include "Meta/MetaManager.h"
#include "Meta/MetaTools.h"

CE::MaterialEditorSystem::MaterialEditorSystem(Material&& asset) :
	AssetEditorSystem(std::move(asset))
{
}

CE::MaterialEditorSystem::~MaterialEditorSystem() = default;

void CE::MaterialEditorSystem::Tick(const float deltaTime)
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

	const MetaType* const materialType = MetaManager::Get().TryGetType<Material>();
	ASSERT(materialType != nullptr);

	MetaAny refToMat{ mAsset };

	for (const MetaField& field : materialType->EachField())
	{
		MetaAny refToMember = field.MakeRef(refToMat);
		ShowInspectUI(std::string{ field.GetName() }, refToMember);
	}
	End();
}

CE::MetaType CE::MaterialEditorSystem::Reflect()
{
	return { MetaType::T<MaterialEditorSystem>{}, "MaterialEditorSystem", 
		MetaType::Base<AssetEditorSystem<Material>>{},
		MetaType::Ctor<Material&&>{} };
}