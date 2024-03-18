#include "Precomp.h"
#include "EditorSystems/AssetEditorSystems/LevelEditorSystem.h"

#include "Utilities/Imgui/WorldInspect.h"
#include "World/World.h"

Engine::LevelEditorSystem::LevelEditorSystem(Level&& asset) :
	AssetEditorSystem(std::move(asset)),
	mWorldHelper(std::make_unique<WorldInspectHelper>(mAsset.CreateWorld(false)))
{
}

Engine::LevelEditorSystem::~LevelEditorSystem() = default;

void Engine::LevelEditorSystem::Tick(const float deltaTime)
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

void Engine::LevelEditorSystem::SaveState(std::ostream& toStream) const
{
	AssetEditorSystem<Level>::SaveState(toStream);

	BinaryGSONObject savedState{};
	mWorldHelper->SaveState(savedState);
	savedState.SaveToBinary(toStream);
}

void Engine::LevelEditorSystem::LoadState(std::istream& fromStream)
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

void Engine::LevelEditorSystem::ApplyChangesToAsset()
{
	mAsset.CreateFromWorld(mWorldHelper->GetWorldBeforeBeginPlay());
}

Engine::MetaType Engine::LevelEditorSystem::Reflect()
{
	return { MetaType::T<LevelEditorSystem>{}, "LevelEditorSystem",
		MetaType::Base<AssetEditorSystem<Level>>{},
		MetaType::Ctor<Level&&>{} };
}
