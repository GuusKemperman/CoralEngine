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
