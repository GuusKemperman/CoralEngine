#include "Precomp.h"
#include "EditorSystems/AssetEditorSystems/PrefabEditorSystem.h"

#include "World/World.h"
#include "World/Registry.h"
#include "Components/TransformComponent.h"
#include "Components/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
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
	{
		const entt::entity camera = reg.Create();
		reg.AddComponent<CameraComponent>(camera);
		reg.AddComponent<FlyCamControllerComponent>(camera);
		reg.AddComponent<NameComponent>(camera, "Camera");
		reg.AddComponent<TransformComponent>(camera).SetLocalPosition({ 0.0f, 0.0f, -30.0f });;
	}

	// And ofcourse some light
	{
		const entt::entity light = reg.Create();
		reg.AddComponent<NameComponent>(light, "Light");
		reg.AddComponent<DirectionalLightComponent>(light);
		reg.AddComponent<TransformComponent>(light).SetLocalOrientation({DEG2RAD(17.0f), DEG2RAD(-63.0f), 0.0f });
	}
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