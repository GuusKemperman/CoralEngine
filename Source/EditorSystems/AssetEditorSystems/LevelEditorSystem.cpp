#include "Precomp.h"
#include "EditorSystems/AssetEditorSystems/LevelEditorSystem.h"

#include "Utilities/Imgui/WorldInspect.h"
#include "World/World.h"

CE::LevelEditorSystem::LevelEditorSystem(Level&& asset) :
	AssetEditorSystem(std::move(asset)),
	mWorldHelper(std::make_unique<WorldInspectHelper>(mAsset.CreateWorld(false)))
{
	mWorldHelper->SwitchToFlyCam();
}

CE::LevelEditorSystem::~LevelEditorSystem() = default;

void CE::LevelEditorSystem::Tick(const float deltaTime)
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

	mWorldHelper->DisplayAndTick(deltaTime);

	End();
}

void CE::LevelEditorSystem::SaveState(std::ostream& toStream) const
{
	AssetEditorSystem<Level>::SaveState(toStream);

	BinaryGSONObject savedState{};
	mWorldHelper->SaveState(savedState);
	savedState.SaveToBinary(toStream);
}

void CE::LevelEditorSystem::LoadState(std::istream& fromStream)
{
	AssetEditorSystem<Level>::LoadState(fromStream);

	BinaryGSONObject savedState{};

	if (!savedState.LoadFromBinary(fromStream))
	{
		LOG(LogEditor, Warning, "Failed to load level editor saved state, which may be fine if this level was an older format (pre 18/03/2024)");
		return;
	}

	mWorldHelper->LoadState(savedState);
}

void CE::LevelEditorSystem::ApplyChangesToAsset()
{
	mAsset.CreateFromWorld(mWorldHelper->GetWorldBeforeBeginPlay());
}

CE::MetaType CE::LevelEditorSystem::Reflect()
{
	return { MetaType::T<LevelEditorSystem>{}, "LevelEditorSystem",
		MetaType::Base<AssetEditorSystem<Level>>{},
		MetaType::Ctor<Level&&>{} };
}
