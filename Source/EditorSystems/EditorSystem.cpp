#include "Precomp.h"
#include "EditorSystems/EditorSystem.h"

#include "Core/Editor.h"
#include "Meta/MetaType.h"

CE::EditorSystem::EditorSystem(const std::string_view name) :
	mName(name)
{
}

bool CE::EditorSystem::Begin(ImGuiWindowFlags flags)
{
	bool open = true;

	ImGui::SetNextWindowSize({ 800.0f, 600.0f }, ImGuiCond_FirstUseEver);
	const bool isCollapsed = !ImGui::Begin(GetName().c_str(), &open, flags);

	if (!open)
	{
		Editor::Get().DestroySystem(GetName());
	}

	return !isCollapsed && open;
}

CE::MetaType CE::EditorSystem::Reflect()
{
	return MetaType{ MetaType::T<EditorSystem>{}, "EditorSystem" };
}
