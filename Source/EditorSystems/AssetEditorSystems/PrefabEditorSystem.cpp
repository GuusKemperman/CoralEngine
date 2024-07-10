#include "Precomp.h"
#include "EditorSystems/AssetEditorSystems/PrefabEditorSystem.h"

#include "World/World.h"
#include "World/Registry.h"
#include "Components/TransformComponent.h"
#include "Components/CameraComponent.h"
#include "Components/FlyCamControllerComponent.h"
#include "Components/NameComponent.h"
#include "Utilities/Imgui/WorldInspect.h"

Engine::PrefabEditorSystem::PrefabEditorSystem(Prefab&& asset) :
	AssetEditorSystem(std::move(asset)),
	mWorldHelper(std::make_unique<WorldInspectHelper>(World{false}))
{
	Registry& reg = mWorldHelper->GetWorld().GetRegistry();
	mPrefabInstance = reg.CreateFromPrefab(mAsset);

	// In case our prefab was invalid
	if (!reg.Valid(mPrefabInstance))
	{
		mPrefabInstance = reg.Create();
		reg.AddComponent<NameComponent>(mPrefabInstance, mAsset.GetName());
	}

	// Make a camera to see the prefab nicely
	const entt::entity camera = reg.Create();
	reg.AddComponent<CameraComponent>(camera);
	reg.AddComponent<FlyCamControllerComponent>(camera);
	reg.AddComponent<NameComponent>(camera, "Camera");
	TransformComponent& cameraTransform = reg.AddComponent<TransformComponent>(camera);

	cameraTransform.SetLocalPosition({ -25.0f, -13.0f, 20.0f });
	cameraTransform.SetLocalOrientation({ 0.0f, 25.0f * (TWOPI / 360.0f), 20.0f * (TWOPI / 360.0f) });
}

Engine::PrefabEditorSystem::~PrefabEditorSystem() = default;

void Engine::PrefabEditorSystem::Tick(const float deltaTime)
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

void Engine::PrefabEditorSystem::ApplyChangesToAsset()
{
	mAsset.CreateFromEntity(mWorldHelper->GetWorldBeforeBeginPlay(), mPrefabInstance);
}

Engine::MetaType Engine::PrefabEditorSystem::Reflect()
{
	return { MetaType::T<PrefabEditorSystem>{}, "PrefabEditorSystem",
		MetaType::Base<AssetEditorSystem<Prefab>>{},
		MetaType::Ctor<Prefab&&>{} };
}