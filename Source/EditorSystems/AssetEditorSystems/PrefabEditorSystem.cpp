#include "Precomp.h"
#include "EditorSystems/AssetEditorSystems/PrefabEditorSystem.h"

#include "Assets/Level.h"
#include "World/World.h"
#include "World/Registry.h"
#include "Components/TransformComponent.h"
#include "Components/NameComponent.h"
#include "Utilities/Imgui/WorldInspect.h"
#include "Utilities/Reflect/ReflectComponentType.h"

namespace
{
	struct PartOfDefaultLevelTag
	{
		friend CE::ReflectAccess;
		static CE::MetaType Reflect()
		{
			using namespace CE;
			MetaType metaType = MetaType{ MetaType::T<PartOfDefaultLevelTag>{}, "PartOfDefaultLevelTag" };
			ReflectComponentType<PartOfDefaultLevelTag>(metaType);
			return metaType;
		}
		REFLECT_AT_START_UP(PartOfDefaultLevelTag);
	};
}

CE::PrefabEditorSystem::PrefabEditorSystem(Prefab&& asset) :
	AssetEditorSystem(std::move(asset)),
	mWorldHelper(std::make_unique<WorldInspectHelper>(Level::CreateDefaultWorld()))
{
	Registry& reg = mWorldHelper->GetWorld().GetRegistry();

	auto entityView = reg.View<entt::entity>();
	reg.AddComponents<PartOfDefaultLevelTag>(entityView.begin(), entityView.end());

	// By default, we will use the flycam
	mWorldHelper->SwitchToFlyCam();

	mPrefabInstance = reg.CreateFromPrefab(mAsset);

	// In case our prefab was invalid
	if (!reg.Valid(mPrefabInstance))
	{
		mPrefabInstance = reg.Create();
		reg.AddComponent<NameComponent>(mPrefabInstance, mAsset.GetName());
	}
}

CE::PrefabEditorSystem::~PrefabEditorSystem() = default;

void CE::PrefabEditorSystem::Tick(const float deltaTime)
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

void CE::PrefabEditorSystem::SaveState(std::ostream& toStream) const
{
	AssetEditorSystem::SaveState(toStream);

	BinaryGSONObject savedState{};
	mWorldHelper->SaveState(savedState);

	savedState.SaveToBinary(toStream);
}

void CE::PrefabEditorSystem::LoadState(std::istream& fromStream)
{
	AssetEditorSystem::LoadState(fromStream);

	BinaryGSONObject savedState{};

	if (!savedState.LoadFromBinary(fromStream))
	{
		LOG(LogEditor, Warning, "Failed to load prefab editor saved state, which may be fine if this prefab was an older format (pre 18/03/2024)");
		return;
	}

	mWorldHelper->LoadState(savedState);
}

void CE::PrefabEditorSystem::ApplyChangesToAsset()
{
	const Registry& reg = mWorldHelper->GetWorldBeforeBeginPlay().GetRegistry();

	if (!reg.Valid(mPrefabInstance))
	{
		// See if there's a new instance
		const auto* entityStorage = reg.Storage<entt::entity>();
		
		for (const auto [entity] : entityStorage->each())
		{
			if (reg.HasComponent<PartOfDefaultLevelTag>(entity))
			{
				continue;
			}

			const TransformComponent* const transform = reg.TryGet<TransformComponent>(entity);

			if (transform == nullptr
				|| transform->GetParent() == nullptr)
			{
				mPrefabInstance = entity;
				break;
			}
		}
	}

	mAsset.CreateFromEntity(mWorldHelper->GetWorldBeforeBeginPlay(), mPrefabInstance);
}

CE::MetaType CE::PrefabEditorSystem::Reflect()
{
	return { MetaType::T<PrefabEditorSystem>{}, "PrefabEditorSystem",
		MetaType::Base<AssetEditorSystem<Prefab>>{},
		MetaType::Ctor<Prefab&&>{} };
}