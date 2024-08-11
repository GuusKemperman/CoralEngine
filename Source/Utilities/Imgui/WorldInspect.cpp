#include "Precomp.h"
#include "Utilities/Imgui/WorldInspect.h"

#include "imgui/imgui_internal.h"

#include "World/World.h"
#include "World/WorldViewport.h"
#include "World/Registry.h"
#include "Rendering/FrameBuffer.h"
#include "Core/Input.h"
#include "Components/TransformComponent.h"
#include "Components/CameraComponent.h"
#include "Components/NameComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Assets/Prefabs/Prefab.h"
#include "Assets/StaticMesh.h"
#include "Assets/Material.h"
#include "Components/FlyCamControllerComponent.h"
#include "Utilities/Imgui/ImguiDragDrop.h"
#include "Utilities/Imgui/ImguiHelpers.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaProps.h"
#include "Utilities/StringFunctions.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "World/Archiver.h"
#include "Utilities/Geometry3d.h"
#include "Utilities/Imgui/WorldDetailsPanel.h"
#include "Utilities/Imgui/WorldHierarchyPanel.h"
#include "Utilities/Imgui/WorldViewportPanel.h"

namespace CE::Internal
{
	struct EditorCameraTag
	{
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(EditorCameraTag);
	};

	struct WasRootCopyTag
	{
		static MetaType Reflect();
	};

	static bool IsStringFromCopyToClipBoard(std::string_view string);

	static const AssetHandle<StaticMesh>& GetMesh(const StaticMeshComponent& component);

	template<typename MeshComponentType>
	static void DoRaycastAgainstMeshComponents(const World& world, Ray3D ray, float& nearestT, entt::entity& nearestEntity);

	// We prepend some form of known string so we can verify whether
	// the clipboard holds copied entities
	static constexpr std::string_view sCopiedEntitiesId = "B1C2FF80";
}

CE::WorldInspectHelper::WorldInspectHelper(std::unique_ptr<World> worldThatHasNotYetBegunPlay) :
	mViewportFrameBuffer(std::make_unique<FrameBuffer>(glm::ivec2(1.f, 1.f))),
	mWorldBeforeBeginPlay(std::move(worldThatHasNotYetBegunPlay))
{
	ASSERT(!mWorldBeforeBeginPlay->HasBegunPlay());
}

CE::WorldInspectHelper::~WorldInspectHelper() = default;

CE::World& CE::WorldInspectHelper::GetWorld()
{
	if (mWorldAfterBeginPlay != nullptr)
	{
		return *mWorldAfterBeginPlay;
	}
	ASSERT(mWorldBeforeBeginPlay != nullptr);
	return *mWorldBeforeBeginPlay;
}

CE::World& CE::WorldInspectHelper::BeginPlay()
{
	if (mWorldAfterBeginPlay != nullptr)
	{
		LOG(LogEditor, Error, "Called begin play when the world has already begun play");
		return GetWorld();
	}
	ASSERT(!mWorldBeforeBeginPlay->HasBegunPlay() && "Do not call BeginPlay on the world yourself, use WorldInspectHelper::BeginPlay");

	mWorldAfterBeginPlay = std::make_unique<World>(false);

	// Duplicate our level world
	const BinaryGSONObject serializedWorld = Archiver::Serialize(*mWorldBeforeBeginPlay);
	Archiver::Deserialize(*mWorldAfterBeginPlay, serializedWorld);

	SwitchToPlayCam();
	mWorldAfterBeginPlay->BeginPlay();
	return GetWorld();
}

CE::World& CE::WorldInspectHelper::EndPlay()
{
	if (mWorldAfterBeginPlay == nullptr)
	{
		LOG(LogEditor, Error, "Called EndPlay, but WorldInspectHelper::BeginPlay was never called");
		return GetWorld();
	}
	ASSERT(mWorldAfterBeginPlay->HasBegunPlay() && "Do not call EndPlay on the world yourself, use WorldInspectHelper::EndPlay");

	mWorldAfterBeginPlay->EndPlay();
	mWorldAfterBeginPlay.reset();

	SwitchToFlyCam();
	return GetWorld();
}

void CE::WorldInspectHelper::DisplayAndTick(const float deltaTime)
{
	mDeltaTimeRunningAverage = mDeltaTimeRunningAverage * sRunningAveragePreservePercentage + deltaTime * (1.0f - sRunningAveragePreservePercentage);

	ImGui::Splitter(true, &mViewportWidth, &mHierarchyAndDetailsWidth);

	if (ImGui::BeginChild("WorldViewport", { mViewportWidth, -2.0f }, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav))
	{
		const ImVec2 firstButtonPos = ImGui::GetWindowContentRegionMin() + ImVec2{ ImGui::GetContentRegionAvail().x / 2.0f - 16.0f, 10.0f };
		const ImVec2 viewportPos = ImGui::GetCursorPos();

		ImDrawList* drawList = ImGui::GetCurrentWindow()->DrawList;

		drawList->ChannelsSplit(2);
		drawList->ChannelsSetCurrent(1);

		if (!mSelectedEntities.empty())
		{
			if (ImGui::RadioButton("Translate", Internal::sGuizmoOperation == ImGuizmo::TRANSLATE))
				Internal::sGuizmoOperation = ImGuizmo::TRANSLATE;
			ImGui::SameLine();
			if (ImGui::RadioButton("Rotate", Internal::sGuizmoOperation == ImGuizmo::ROTATE))
				Internal::sGuizmoOperation = ImGuizmo::ROTATE;
			ImGui::SameLine();
			if (ImGui::RadioButton("Scale", Internal::sGuizmoOperation == ImGuizmo::SCALE))
				Internal::sGuizmoOperation = ImGuizmo::SCALE;

			if (Internal::sGuizmoOperation != ImGuizmo::SCALE)
			{
				if (ImGui::RadioButton("Local", Internal::sGuizmoMode == ImGuizmo::LOCAL))
					Internal::sGuizmoMode = ImGuizmo::LOCAL;
				ImGui::SameLine();
				if (ImGui::RadioButton("World", Internal::sGuizmoMode == ImGuizmo::WORLD))
					Internal::sGuizmoMode = ImGuizmo::WORLD;
			}

			ImGui::Checkbox("Snap", &Internal::sShouldGuizmoSnap);
			if (Input::Get().WasKeyboardKeyReleased(Input::KeyboardKey::LeftControl))
			{
				Internal::sShouldGuizmoSnap = Internal::sShouldGuizmoSnapPrevious;
			}
			if (Input::Get().IsKeyboardKeyHeld(Input::KeyboardKey::LeftControl))
			{
				Internal::sShouldGuizmoSnap = true;
			}
			else
			{
				Internal::sShouldGuizmoSnapPrevious = Internal::sShouldGuizmoSnap;
			}
			ImGui::SameLine();

			if (Internal::sShouldGuizmoSnap)
			{
				switch (Internal::sGuizmoOperation)
				{
				case ImGuizmo::TRANSLATE:
					Internal::sCurrentSnapTo = &Internal::sTranslateSnapTo;
					break;
				case ImGuizmo::ROTATE:
					Internal::sCurrentSnapTo = &Internal::sRotationSnapTo;
					break;
				case ImGuizmo::SCALE:
					Internal::sCurrentSnapTo = &Internal::sScaleSnapTo;
					break;
				default:
					break;
				}
				// Convert to string and truncate to the desired decimal precision.
				std::string valueStr = std::to_string(*Internal::sCurrentSnapTo);
				valueStr = valueStr.substr(0, valueStr.find('.') + 1 + 3); // 1 - dot character and 3 - decimal precision 
				// Calculate the text size with some padding
				const ImVec2 textSize = ImGui::CalcTextSize(valueStr.c_str());
				const float padding = ImGui::GetStyle().FramePadding.x * 2.0f;

				const float width = textSize.x + padding;
				ImGui::SetNextItemWidth(width);
				ImGui::InputFloat("##", Internal::sCurrentSnapTo);
			}
		}

		ImGui::SetCursorPos(firstButtonPos);
		ImGui::SetNextItemAllowOverlap();

		if (!GetWorld().HasBegunPlay())
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 0.0f, 1.0f, 0.0f, 1.0f });
			if (ImGui::Button(ICON_FA_PLAY))
			{
				(void)BeginPlay();
			}
			ImGui::PopStyleColor();
			ImGui::SetItemTooltip("Begin play");
		}
		else if (!GetWorld().IsPaused())
		{
			if (ImGui::Button(ICON_FA_PAUSE))
			{
				GetWorld().Pause();
			}
			ImGui::SetItemTooltip("Pause");
		}
		else
		{
			if (ImGui::Button(ICON_FA_PLAY))
			{
				GetWorld().Unpause();
			}
			ImGui::SetItemTooltip("Resume play");
		}

		ImGui::SameLine();
		ImGui::SetCursorPosY(firstButtonPos.y);
		ImGui::SetNextItemAllowOverlap();

		if (GetWorld().HasBegunPlay())
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 1.0f, 0.0f, 0.0f, 1.0f });
			if (ImGui::Button(ICON_FA_STOP))
			{
				(void)EndPlay();
			}
			ImGui::PopStyleColor();
			ImGui::SetItemTooltip("Stop");
		}
		else
		{
			ImGui::BeginDisabled();
			ImGui::Button(ICON_FA_STOP);
			ImGui::EndDisabled();
		}

		ImGui::SameLine();
		ImGui::SetCursorPosY(firstButtonPos.y);
		ImGui::SetNextItemAllowOverlap();

		if (mSelectedCameraBeforeWeSwitchedToFlyCam.has_value())
		{
			if (ImGui::Button(ICON_FA_GAMEPAD))
			{
				SwitchToPlayCam();
			}
			ImGui::SetItemTooltip("Switch to the play camera");
		}
		else
		{
			if (ImGui::Button(ICON_FA_EJECT))
			{
				SwitchToFlyCam();
			}
			ImGui::SetItemTooltip("Switch to the fly camera");
		}

		// World will not change anymore
		World& world = GetWorld();
		World::PushWorld(world);

		const glm::vec2 fpsCursorPos = { viewportPos.x + mViewportWidth - 60.0f, 0.0f };
		ImGui::SetCursorPos(fpsCursorPos);

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.3f);
		ImGui::TextUnformatted(Format("FPS {:.1f}", 1.0f / mDeltaTimeRunningAverage).data());

		const std::string viewportSizeText = Format("{}x{}", static_cast<int>(world.GetViewport().GetViewportSize().x), static_cast<int>(world.GetViewport().GetViewportSize().y));
		ImGui::SetCursorPosX(viewportPos.x + mViewportWidth - ImGui::CalcTextSize(viewportSizeText.c_str()).x - 10.0f);

		ImGui::TextUnformatted(viewportSizeText.c_str());
		ImGui::PopStyleVar();

		{
			const auto possibleCamerasView = world.GetRegistry().View<CameraComponent>();

			if (possibleCamerasView.size() > 1)
			{
				const entt::entity cameraEntity = CameraComponent::GetSelected(world);

				ImGui::SetCursorPos({ fpsCursorPos.x - ImGui::CalcTextSize(ICON_FA_CAMERA).x - 10.0f, fpsCursorPos.y });

				if (ImGui::Button(ICON_FA_CAMERA))
				{
					ImGui::OpenPopup("CameraSelectPopUp");
				}

				if (ImGui::BeginPopup("CameraSelectPopUp"))
				{
					for (entt::entity possibleCamera : possibleCamerasView)
					{
						if (ImGui::MenuItem(NameComponent::GetDisplayName(world.GetRegistry(), possibleCamera).data(), nullptr, possibleCamera == cameraEntity))
						{
							CameraComponent::Select(world, possibleCamera);
						}
					}
					ImGui::EndPopup();
				}
			}
		}

		drawList->ChannelsSetCurrent(0);
		ImGui::SetCursorPos(viewportPos);

		if (mSelectedCameraBeforeWeSwitchedToFlyCam.has_value())
		{
			SpawnFlyCam();
		}

		world.Tick(deltaTime);
		WorldViewportPanel::Display(world, *mViewportFrameBuffer, &mSelectedEntities);

		if (mSelectedCameraBeforeWeSwitchedToFlyCam.has_value())
		{
			DestroyFlyCam();
		}

		drawList->ChannelsMerge();

		World::PopWorld();

		if (world.HasRequestedEndPlay())
		{
			(void)EndPlay();
		}
	}
	ImGui::EndChild();

	ImGui::SameLine();

	if (ImGui::BeginChild("HierarchyAndDetailsWindow", { mHierarchyAndDetailsWidth, 0.0f }, false, ImGuiWindowFlags_NoScrollbar))
	{
		World& world = GetWorld();
		World::PushWorld(world);

		ImGui::PushID(2); // Second splitter requires new ID
		ImGui::Splitter(false, &mHierarchyHeight, &mDetailsHeight);
		ImGui::PopID();

		ImGui::BeginChild("WorldHierarchy", { 0.0f, mHierarchyHeight }, false, ImGuiWindowFlags_NoScrollbar);
		WorldHierarchy::Display(world, &mSelectedEntities);
		ImGui::EndChild();

		ImGui::BeginChild("WorldDetails", { 0.0f, mDetailsHeight - 5.0f }, false, ImGuiWindowFlags_NoScrollbar);
		WorldDetails::Display(world, mSelectedEntities);
		ImGui::EndChild();

		World::PopWorld();
	}

	ImGui::EndChild();
}

void CE::WorldInspectHelper::SaveState(BinaryGSONObject& state)
{
	state.AddGSONMember("selected") << mSelectedEntities;
	state.AddGSONMember("hierarchyWidth") << mHierarchyHeight;
	state.AddGSONMember("detailsWidth") << mDetailsHeight;
	state.AddGSONMember("viewportWidth") << mViewportWidth;
	state.AddGSONMember("hierarchyAndDetailsWidth") << mHierarchyAndDetailsWidth;
	state.AddGSONMember("selectedCam") << mSelectedCameraBeforeWeSwitchedToFlyCam;
	state.AddGSONMember("flycamWorldMat") << mFlyCamWorldMatrix;
}

void CE::WorldInspectHelper::LoadState(const BinaryGSONObject& state)
{
	const BinaryGSONMember* const selected = state.TryGetGSONMember("selected");
	const BinaryGSONMember* const hierarchyWidth = state.TryGetGSONMember("hierarchyWidth");
	const BinaryGSONMember* const detailsWidth = state.TryGetGSONMember("detailsWidth");
	const BinaryGSONMember* const viewportWidth = state.TryGetGSONMember("viewportWidth");
	const BinaryGSONMember* const hierarchyAndDetailsWidth = state.TryGetGSONMember("hierarchyAndDetailsWidth");

	if (selected == nullptr
		|| hierarchyWidth == nullptr
		|| detailsWidth == nullptr
		|| viewportWidth == nullptr
		|| hierarchyAndDetailsWidth == nullptr)
	{
		LOG(LogEditor, Verbose, "Could not load state for world inspect helper, missing values");
		return;
	}

	*selected >> mSelectedEntities;
	*hierarchyWidth >> mHierarchyHeight;
	*detailsWidth >> mDetailsHeight;
	*viewportWidth >> mViewportWidth;
	*hierarchyAndDetailsWidth >> mHierarchyAndDetailsWidth;

	const BinaryGSONMember* const selectedCam = state.TryGetGSONMember("selectedCam");

	if (selectedCam != nullptr)
	{
		*selectedCam >> mSelectedCameraBeforeWeSwitchedToFlyCam;
	}

	const BinaryGSONMember* const flycamWorldMat = state.TryGetGSONMember("flycamWorldMat");

	if (flycamWorldMat != nullptr)
	{
		*flycamWorldMat >> mFlyCamWorldMatrix;
	}
}

void CE::WorldInspectHelper::SaveFlyCam()
{
	Registry& reg = GetWorld().GetRegistry();

	entt::entity flyCam = reg.View<Internal::EditorCameraTag>().front();
	mFlyCamWorldMatrix = {};

	if (flyCam == entt::null)
	{
		return;
	}

	const TransformComponent* const transform = reg.TryGet<TransformComponent>(flyCam);

	if (transform != nullptr)
	{
		mFlyCamWorldMatrix = transform->GetWorldMatrix();
	}
}

void CE::WorldInspectHelper::SwitchToFlyCam()
{
	mSelectedCameraBeforeWeSwitchedToFlyCam = CameraComponent::GetSelected(GetWorld());
}

void CE::WorldInspectHelper::SwitchToPlayCam()
{
	Registry& reg = GetWorld().GetRegistry();

	for (const entt::entity entity : reg.View<Internal::EditorCameraTag>())
	{
		reg.Destroy(entity, true);
	}
	reg.RemovedDestroyed();

	if (mSelectedCameraBeforeWeSwitchedToFlyCam.has_value()
		&& reg.Valid(*mSelectedCameraBeforeWeSwitchedToFlyCam))
	{
		CameraComponent::Select(GetWorld(), *mSelectedCameraBeforeWeSwitchedToFlyCam);
		mFlyCamWorldMatrix = {};
	}

	mSelectedCameraBeforeWeSwitchedToFlyCam.reset();
}

void CE::WorldInspectHelper::SpawnFlyCam()
{
	Registry& reg = GetWorld().GetRegistry();

	for (const entt::entity entity : reg.View<Internal::EditorCameraTag>())
	{
		reg.Destroy(entity, true);
	}
	reg.RemovedDestroyed();

	const entt::entity flyCam = reg.Create();
	reg.AddComponent<CameraComponent>(flyCam);
	reg.AddComponent<FlyCamControllerComponent>(flyCam);
	reg.AddComponent<Internal::EditorCameraTag>(flyCam);
	reg.AddComponent<TransformComponent>(flyCam);

	if (mFlyCamWorldMatrix == glm::mat4{})
	{
		const TransformComponent* playCamera = reg.TryGet<TransformComponent>(mSelectedCameraBeforeWeSwitchedToFlyCam.value_or(entt::null));

		if (playCamera != nullptr)
		{
			reg.Get<TransformComponent>(flyCam).SetWorldMatrix(playCamera->GetWorldMatrix());
		}
	}
	else
	{
		reg.Get<TransformComponent>(flyCam).SetWorldMatrix(mFlyCamWorldMatrix);
	}

	CameraComponent::Select(GetWorld(), flyCam);
}

void CE::WorldInspectHelper::DestroyFlyCam()
{
	SaveFlyCam();

	Registry& reg = GetWorld().GetRegistry();

	for (const entt::entity entity : reg.View<Internal::EditorCameraTag>())
	{
		reg.Destroy(entity, true);
	}
	reg.RemovedDestroyed();

	CameraComponent::Select(GetWorld(), mSelectedCameraBeforeWeSwitchedToFlyCam.value_or(entt::null));
}

entt::entity CE::Internal::GetEntityThatMouseIsHoveringOver(const World& world)
{
	const entt::entity camera = CameraComponent::GetSelected(world);

	if (camera == entt::null)
	{
		return entt::null;
	}

	const TransformComponent* transform = world.GetRegistry().TryGet<TransformComponent>(camera);

	if (transform == nullptr)
	{
		return entt::null;
	}

	const Ray3D ray{ transform->GetWorldPosition(), world.GetViewport().GetScreenToWorldDirection(Input::Get().GetMousePosition()) };

	float nearestT = std::numeric_limits<float>::infinity();
	entt::entity nearestEntity = entt::null;

	DoRaycastAgainstMeshComponents<StaticMeshComponent>(world, ray, nearestT, nearestEntity);

	return nearestEntity;
}

void CE::Internal::RemoveInvalidEntities(CE::World& world, std::vector<entt::entity>& selectedEntities)
{
	selectedEntities.erase(std::remove_if(selectedEntities.begin(), selectedEntities.end(),
		[&world](const entt::entity& entity)
		{
			return !world.GetRegistry().Valid(entity);
		}), selectedEntities.end());
}

bool CE::Internal::ToggleIsEntitySelected(std::vector<entt::entity>& selectedEntities, entt::entity toSelect)
{
	if (!Input::Get().IsKeyboardKeyHeld(Input::KeyboardKey::LeftControl, false))
	{
		selectedEntities.clear();
	}

	const auto it = std::find(selectedEntities.begin(), selectedEntities.end(), toSelect);

	if (it == selectedEntities.end())
	{
		selectedEntities.push_back(toSelect);
		return true;
	}

	selectedEntities.erase(it);
	
	return false;
}

void CE::Internal::DeleteEntities(World& world, std::vector<entt::entity>& selectedEntities)
{
	world.GetRegistry().Destroy(selectedEntities.begin(), selectedEntities.end(), true);
	selectedEntities.clear();
}

std::string CE::Internal::CopyToClipBoard(const World& world, const std::vector<entt::entity>& selectedEntities)
{
	if (selectedEntities.empty())
	{
		return {};
	}

	// Const_cast is fine, we remove the changes immediately afterwards.
	// We just want the tags to be serialized so we know what to return in the Paste function.
	Registry& reg = const_cast<Registry&>(world.GetRegistry());

	reg.AddComponents<WasRootCopyTag>(selectedEntities.begin(), selectedEntities.end());
	BinaryGSONObject object = Archiver::Serialize(world, selectedEntities, true);
	reg.RemoveComponents<WasRootCopyTag>(selectedEntities.begin(), selectedEntities.end());

	std::ostringstream strStream{};

	object.SaveToBinary(strStream);

	// Clipboards work with c strings,
	// since our binary data might contain a
	// \0 character, we convert it to HEX first
	const std::string clipBoardData = std::string{ sCopiedEntitiesId } + StringFunctions::BinaryToHex(strStream.str());
	ImGui::SetClipboardText(clipBoardData.c_str());
	return clipBoardData;
}

void CE::Internal::CutToClipBoard(World& world, std::vector<entt::entity>& selectedEntities)
{
	CopyToClipBoard(world, selectedEntities);
	DeleteEntities(world, selectedEntities);
}

void CE::Internal::PasteClipBoard(World& world, std::vector<entt::entity>& selectedEntities, std::string_view clipBoardData)
{
	if (!IsStringFromCopyToClipBoard(clipBoardData))
	{
		return;
	}

	// Force reflection, if it hasn't been already
	(void)MetaManager::Get().GetType<WasRootCopyTag>();

	BinaryGSONObject object{};

	{
		const std::string binaryCopiedEntities = StringFunctions::HexToBinary(clipBoardData.substr(sCopiedEntitiesId.size()));
		view_istream stream{ binaryCopiedEntities };

		if (!object.LoadFromBinary(stream))
		{
			LOG(LogWorld, Error, "Trying to paste entities, but the provided string was unexpectedly invalid");
			return;
		}
	}

	selectedEntities = Archiver::Deserialize(world, object);

	Registry& reg = world.GetRegistry();

	// We only return the entities that were passed into the Paste function,
	// otherwise we would be returning the children, and thats difficult for
	// the caller to sort out.
	selectedEntities.erase(std::remove_if(selectedEntities.begin(), selectedEntities.end(),
		[&reg](entt::entity entity)
		{
			return !reg.HasComponent<WasRootCopyTag>(entity);
		}), selectedEntities.end());

	// The tag got copied as well, lets remove that as well
	reg.RemoveComponents<WasRootCopyTag>(selectedEntities.begin(), selectedEntities.end());

	// Append the number to the end
	for (entt::entity copy : selectedEntities)
	{
		NameComponent* name = reg.TryGet<NameComponent>(copy);

		if (name == nullptr)
		{
			continue;
		}

		name->mName = StringFunctions::CreateUniqueName(name->mName,
			[&reg](std::string_view copyName)
			{
				for (auto [entity, existingNameComponent] : reg.View<const NameComponent>().each())
				{
					if (copyName == existingNameComponent.mName)
					{
						return false;
					}
				}

				return true;
			});
	}
}

void CE::Internal::Duplicate(World& world, std::vector<entt::entity>& selectedEntities)
{
	const std::string clipboardData = CopyToClipBoard(world, selectedEntities);
	PasteClipBoard(world, selectedEntities, clipboardData);
}

CE::MetaType CE::Internal::EditorCameraTag::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<EditorCameraTag>{}, "EditorCameraTag" };
	ReflectComponentType<EditorCameraTag>(metaType);
	return metaType;
}

CE::MetaType CE::Internal::WasRootCopyTag::Reflect()
{
	MetaType type{ MetaType::T<WasRootCopyTag>{}, "WasRootCopyTag" };
	CE::ReflectComponentType<WasRootCopyTag>(type);
	return type;
}

std::optional<std::string_view> CE::Internal::GetSerializedEntitiesInClipboard()
{
	const char* clipBoardDataCStr = ImGui::GetClipboardText();

	if (clipBoardDataCStr == nullptr)
	{
		return std::nullopt;
	}

	const std::string_view clipBoardData{ clipBoardDataCStr };

	if (IsStringFromCopyToClipBoard(clipBoardData))
	{
		return clipBoardData;
	}

	return std::nullopt;
}

bool CE::Internal::IsStringFromCopyToClipBoard(std::string_view string)
{
	return string.substr(0, sCopiedEntitiesId.size()) == sCopiedEntitiesId;
}

const CE::AssetHandle<CE::StaticMesh>& CE::Internal::GetMesh(const StaticMeshComponent& component)
{
	return component.mStaticMesh;
}

template <typename MeshComponentType>
void CE::Internal::DoRaycastAgainstMeshComponents(const World& world, Ray3D ray, float& nearestT,
	entt::entity& nearestEntity)
{
	static std::vector<glm::vec3> transformedPoints{};

	for (auto [entity, transform, meshComponent] : world.GetRegistry().View<TransformComponent, MeshComponentType>().each())
	{
		auto mesh = GetMesh(meshComponent);

		if (mesh == nullptr
			|| meshComponent.mMaterial == nullptr)
		{
			continue;
		}

		const glm::mat4 worldMat = transform.GetWorldMatrix();
		const glm::mat4 inversedWorldMat = glm::inverse(worldMat);
		CE::Ray3D rayInMeshLocalSpace = { inversedWorldMat * glm::vec4{ ray.mOrigin, 1.0f }, inversedWorldMat * glm::vec4{ ray.mDirection, 0.0f } };
		rayInMeshLocalSpace.mDirection = glm::normalize(rayInMeshLocalSpace.mDirection);

		if (!CE::AreOverlapping(rayInMeshLocalSpace, mesh->GetBoundingBox()))
		{
			continue;
		}

		transformedPoints.clear();
		transformedPoints.insert(transformedPoints.end(), mesh->GetVertices().begin(), mesh->GetVertices().end());

		for (glm::vec3& point : transformedPoints)
		{
			point = worldMat * glm::vec4{ point, 1.0f };
		}

		const auto& indices = mesh->GetIndices();

		for (uint32 i = 0; i < indices.size(); i += 3)
		{
			const float t = TimeOfRayIntersection(ray, { transformedPoints[indices[i]], transformedPoints[indices[i + 1]], transformedPoints[indices[i + 2]] });

			if (t < nearestT)
			{
				nearestT = t;
				nearestEntity = entity;
				// Don't break because some triangles may be even
				// closer to the camera.
			}
		}
	}
}

void CE::Internal::CheckShortcuts(World& world, std::vector<entt::entity>& selectedEntities, ShortCutType types)
{
	RemoveInvalidEntities(world, selectedEntities);

	if (types & ShortCutType::Delete
		&& Input::Get().WasKeyboardKeyReleased(Input::KeyboardKey::Delete))
	{
		DeleteEntities(world, selectedEntities);
	}

	if (Input::Get().IsKeyboardKeyHeld(Input::KeyboardKey::LeftControl)
		|| Input::Get().IsKeyboardKeyHeld(Input::KeyboardKey::RightControl))
	{
		if (types & ShortCutType::CopyPaste)
		{
			if (Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::C))
			{
				CopyToClipBoard(world, selectedEntities);
			}

			std::optional<std::string_view> copiedEntities = GetSerializedEntitiesInClipboard();
			if (copiedEntities.has_value()
				&& Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::V))
			{
				PasteClipBoard(world, selectedEntities, *copiedEntities);
			}

			if (Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::X))
			{
				CutToClipBoard(world, selectedEntities);
			}

			if (Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::D))
			{
				Duplicate(world, selectedEntities);
			}
		}
	

		if (types & ShortCutType::SelectDeselect
			&& Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::A))
		{
			const auto& entityStorage = world.GetRegistry().Storage<entt::entity>();
			selectedEntities = { entityStorage.begin(), entityStorage.end() };
		}
	}

	if (types & ShortCutType::SelectDeselect
		&& Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::Escape))
	{
		selectedEntities.clear();
	}

	if (types & ShortCutType::GuizmoModes)
	{
		if (Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::E))
		{
			sGuizmoOperation = ImGuizmo::SCALE;
		}

		if (Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::R))
		{
			sGuizmoOperation = ImGuizmo::ROTATE;
		}

		if (Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::T))
		{
			sGuizmoOperation = ImGuizmo::TRANSLATE;
		}
	}
}

void CE::Internal::ReceiveDragDrops(World& world)
{
	WeakAssetHandle<Prefab> receivedPrefab = DragDrop::PeekAsset<Prefab>();

	if (receivedPrefab != nullptr
		&& DragDrop::AcceptAsset())
	{
		world.GetRegistry().CreateFromPrefab(*AssetHandle<Prefab>{ receivedPrefab });
	}

	WeakAssetHandle<StaticMesh> receivedMesh = DragDrop::PeekAsset<StaticMesh>();

	if (receivedMesh != nullptr
		&& DragDrop::AcceptAsset())
	{
		Registry& reg = world.GetRegistry();
		entt::entity entity = reg.Create();

		reg.AddComponent<TransformComponent>(entity);
		StaticMeshComponent& meshComponent = reg.AddComponent<StaticMeshComponent>(entity);
		meshComponent.mStaticMesh = AssetHandle<StaticMesh>{ receivedMesh };
		meshComponent.mMaterial = Material::TryGetDefaultMaterial();
	}
}
