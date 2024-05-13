#include "Precomp.h"
#include "Utilities/Imgui/WorldInspect.h"

#include "imgui/ImGuizmo.h"
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
#include "Assets/SkinnedMesh.h"
#include "Assets/Material.h"
#include "Components/ComponentFilter.h"
#include "Components/SkinnedMeshComponent.h"
#include "Utilities/Imgui/ImguiDragDrop.h"
#include "Utilities/Imgui/ImguiInspect.h"
#include "Utilities/Imgui/ImguiHelpers.h"
#include "Utilities/Search.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaProps.h"
#include "Core/AssetManager.h"
#include "Utilities/StringFunctions.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "World/Archiver.h"
#include "Rendering/Renderer.h"
#include "Utilities/Geometry3d.h"

namespace
{
	ImGuizmo::OPERATION sGuizmoOperation{ ImGuizmo::OPERATION::TRANSLATE };
	ImGuizmo::MODE sGuizmoMode{ ImGuizmo::MODE::WORLD };
	bool sShouldGuizmoSnap{};

	// Different variable for each operation.
	float sTranslateSnapTo{ 1.0f };
	float sRotationSnapTo{ 30.0f };
	float sScaleSnapTo{ 1.0f };

	// Storing it so that we only have to do the check once.
	float* sCurrentSnapTo{};

	// We need this for the Manipulate() function
	// which expects a vec3 in the form of a pointer to float.
	glm::vec3 sCurrentSnapToVec3{};

	void RemoveInvalidEntities(CE::World& world, std::vector<entt::entity>& selectedEntities);
	void ToggleIsEntitySelected(std::vector<entt::entity>& selectedEntities, entt::entity toSelect);

	void DeleteEntities(CE::World& world, std::vector<entt::entity>& selectedEntities);
	std::string CopyToClipBoard(const CE::World& world, const std::vector<entt::entity>& selectedEntities);
	void CutToClipBoard(CE::World& world, std::vector<entt::entity>& selectedEntities);
	void PasteClipBoard(CE::World& world, std::vector<entt::entity>& selectedEntities, std::string_view clipboardData);
	void Duplicate(CE::World& world, std::vector<entt::entity>& selectedEntities);
	std::optional<std::string_view> GetSerializedEntitiesInClipboard();
	bool IsStringFromCopyToClipBoard(std::string_view string);

	void CheckShortcuts(CE::World& world, std::vector<entt::entity>& selectedEntities);
	void ReceiveDragDrops(CE::World& world);
}

CE::WorldInspectHelper::WorldInspectHelper(World&& worldThatHasNotYetBegunPlay) :
	mViewportFrameBuffer(std::make_unique<FrameBuffer>()),
	mWorldBeforeBeginPlay(std::make_unique<World>(std::move(worldThatHasNotYetBegunPlay)))
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

	return GetWorld();
}

void CE::WorldInspectHelper::DisplayAndTick(const float deltaTime)
{
	mDeltaTimeRunningAverage = mDeltaTimeRunningAverage * sRunningAveragePreservePercentage + deltaTime * (1.0f - sRunningAveragePreservePercentage);

	ImGui::Splitter(true, &mViewportWidth, &mHierarchyAndDetailsWidth);

	if (ImGui::BeginChild("WorldViewport", { mViewportWidth, -2.0f }, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav))
	{
		const ImVec2 beginPlayPos = ImGui::GetWindowContentRegionMin() + ImVec2{ ImGui::GetContentRegionAvail().x / 2.0f, 10.0f };
		const ImVec2 viewportPos = ImGui::GetCursorPos();

		ImDrawList* drawList = ImGui::GetCurrentWindow()->DrawList;

		drawList->ChannelsSplit(2);

		drawList->ChannelsSetCurrent(1);

		if (!mSelectedEntities.empty())
		{

			if (ImGui::RadioButton("Translate", sGuizmoOperation == ImGuizmo::TRANSLATE))
				sGuizmoOperation = ImGuizmo::TRANSLATE;
			ImGui::SameLine();
			if (ImGui::RadioButton("Rotate", sGuizmoOperation == ImGuizmo::ROTATE))
				sGuizmoOperation = ImGuizmo::ROTATE;
			ImGui::SameLine();
			if (ImGui::RadioButton("Scale", sGuizmoOperation == ImGuizmo::SCALE))
				sGuizmoOperation = ImGuizmo::SCALE;

			if (sGuizmoOperation != ImGuizmo::SCALE)
			{
				if (ImGui::RadioButton("Local", sGuizmoMode == ImGuizmo::LOCAL))
					sGuizmoMode = ImGuizmo::LOCAL;
				ImGui::SameLine();
				if (ImGui::RadioButton("World", sGuizmoMode == ImGuizmo::WORLD))
					sGuizmoMode = ImGuizmo::WORLD;
			}

			ImGui::Checkbox("Snap", &sShouldGuizmoSnap);
			if (Input::Get().IsKeyboardKeyHeld(Input::KeyboardKey::LeftControl))
			{
				sShouldGuizmoSnap = true;
			}
			if (Input::Get().WasKeyboardKeyReleased(Input::KeyboardKey::LeftControl))
			{
				sShouldGuizmoSnap = false;
			}
			ImGui::SameLine();

			if (sShouldGuizmoSnap)
			{
				switch (sGuizmoOperation)
				{
				case ImGuizmo::TRANSLATE:
					sCurrentSnapTo = &sTranslateSnapTo;
					break;
				case ImGuizmo::ROTATE:
					sCurrentSnapTo = &sRotationSnapTo;
					break;
				case ImGuizmo::SCALE:
					sCurrentSnapTo = &sScaleSnapTo;
					break;
				default:
					break;
				}
				// Convert to string and truncate to the desired decimal precision.
				std::string valueStr = std::to_string(*sCurrentSnapTo);
				valueStr = valueStr.substr(0, valueStr.find('.') + 1 + 3); // 1 - dot character and 3 - decimal precision 
				// Calculate the text size with some padding
				const ImVec2 textSize = ImGui::CalcTextSize(valueStr.c_str());
				const float padding = ImGui::GetStyle().FramePadding.x * 2.0f;

				const float width =  textSize.x + padding;
				ImGui::SetNextItemWidth(width);
				ImGui::InputFloat("##", sCurrentSnapTo);
			}
		}

		ImGui::SetCursorPos(beginPlayPos);

		if (!GetWorld().HasBegunPlay())
		{
			ImGui::SetNextItemAllowOverlap();
			if (ImGui::Button(ICON_FA_PLAY))
			{
				(void)BeginPlay();
			}
			ImGui::SetItemTooltip("Begin play");
		}
		else
		{
			if (GetWorld().IsPaused())
			{
				ImGui::SetNextItemAllowOverlap();
				if (ImGui::Button(ICON_FA_PLAY))
				{
					GetWorld().Unpause();
				}
				ImGui::SetItemTooltip("Resume play");
			}
			else
			{
				ImGui::SetNextItemAllowOverlap();
				if (ImGui::Button(ICON_FA_PAUSE))
				{
					GetWorld().Pause();
				}
				ImGui::SetItemTooltip("Pause");
			}

			ImGui::SameLine();
			ImGui::SetCursorPosY(beginPlayPos.y);

			ImGui::SetNextItemAllowOverlap();
			if (ImGui::Button(ICON_FA_STOP))
			{
				(void)EndPlay();
			}
			ImGui::SetItemTooltip("Stop");
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
						if (ImGui::MenuItem(NameComponent::GetDisplayName(world.GetRegistry(), possibleCamera).c_str(), nullptr, possibleCamera == cameraEntity))
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

		world.Tick(deltaTime);

		WorldViewportPanel::Display(world, *mViewportFrameBuffer, &mSelectedEntities);
		drawList->ChannelsMerge();

		World::PopWorld();
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

	CheckShortcuts(GetWorld(), mSelectedEntities);
}

void CE::WorldInspectHelper::SaveState(BinaryGSONObject& state)
{
	state.AddGSONMember("selected") << mSelectedEntities;
	state.AddGSONMember("hierarchyWidth") << mHierarchyHeight;
	state.AddGSONMember("detailsWidth") << mDetailsHeight;
	state.AddGSONMember("viewportWidth") << mViewportWidth;
	state.AddGSONMember("hierarchyAndDetailsWidth") << mHierarchyAndDetailsWidth;
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
}

void CE::WorldViewportPanel::Display(World& world, FrameBuffer& frameBuffer,
	std::vector<entt::entity>* selectedEntities)
{
	const glm::vec2 windowPos = ImGui::GetWindowPos();
	const glm::vec2 contentMin = ImGui::GetWindowContentRegionMin();
	const glm::vec2 contentSize = ImGui::GetContentRegionAvail();

	if (contentSize.x <= 0.0f
		|| contentSize.y <= 0.0f)
	{
		return;
	}

	std::vector<entt::entity> dummySelectedEntities{};
	if (selectedEntities == nullptr)
	{
		selectedEntities = &dummySelectedEntities;
	}// From here on out, we can assume selectedEntities != nullptr

	RemoveInvalidEntities(world, *selectedEntities);

	const entt::entity cameraOwner = CameraComponent::GetSelected(world);

	if (cameraOwner == entt::null)
	{
		ImGui::TextUnformatted("No camera");
		return;
	}

	ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
	SetGizmoRect(windowPos + contentMin, contentSize);

	Renderer::Get().RenderToFrameBuffer(world, frameBuffer, contentSize);

	ImGui::SetCursorPos(contentMin);

	ImGui::Image((ImTextureID)frameBuffer.GetColorTextureId(),
		ImVec2(contentSize),
		ImVec2(0, 0),
		ImVec2(1, 1));

	bool shouldSelectEntityUnderneathMouse = ImGui::IsItemClicked();

	// Since it is our 'image' that receives the drag drop, we call this right after the image call.
	ReceiveDragDrops(world);

	ImGui::SetCursorPos(contentMin);

	// There is no need to try to draw gizmos/manipulate transforms when nothing is selected
	if (!selectedEntities->empty())
	{
		ShowComponentGizmos(world, *selectedEntities);

		// We don't change the selection if we are interacting with the gizmos
		shouldSelectEntityUnderneathMouse &= !GizmoManipulateSelectedTransforms(world, *selectedEntities, world.GetRegistry().Get<CameraComponent>(cameraOwner));
	}

	if (shouldSelectEntityUnderneathMouse)
	{
		const entt::entity hoveringOver = GetEntityThatMouseIsHoveringOver(world);

		if (hoveringOver != entt::null)
		{
			ToggleIsEntitySelected(*selectedEntities, hoveringOver);
		}
	}
}

namespace
{
	const CE::AssetHandle<CE::StaticMesh>& GetMesh(const CE::StaticMeshComponent& component)
	{
		return component.mStaticMesh;
	}

	const CE::AssetHandle<CE::SkinnedMesh>& GetMesh(const CE::SkinnedMeshComponent& component)
	{
		return component.mSkinnedMesh;
	}

	template<typename MeshComponentType>
	void DoRaycastAgainstMeshComponents(const CE::World& world, CE::Ray3D ray, float& nearestT, entt::entity& nearestEntity)
	{
		static std::vector<glm::vec3> transformedPoints{};

		for (auto [entity, transform, meshComponent] : world.GetRegistry().View<CE::TransformComponent, MeshComponentType>().each())
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
}

entt::entity CE::WorldViewportPanel::GetEntityThatMouseIsHoveringOver(const World& world)
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
	DoRaycastAgainstMeshComponents<SkinnedMeshComponent>(world, ray, nearestT, nearestEntity);

	return nearestEntity;
}

void CE::WorldViewportPanel::ShowComponentGizmos(World& world, const std::vector<entt::entity>& selectedEntities)
{
	Registry& reg = world.GetRegistry();

	for (auto&& [typeHash,storage] : reg.Storage())
	{
		const MetaType* const type = MetaManager::Get().TryGetType(typeHash);

		if (type == nullptr)
		{
			continue;
		}

		const MetaFunc* const event = TryGetEvent(*type, sDrawGizmoEvent);

		if (event == nullptr)
		{
			continue;
		}

		const bool isStatic = event->GetProperties().Has(Props::sIsEventStaticTag);

		for (entt::entity entity : selectedEntities)
		{
			if (!storage.contains(entity))
			{
				continue;
			}

			if (isStatic)
			{
				event->InvokeUncheckedUnpacked(world, entity);
			}
			else
			{
				MetaAny component{ *type, storage.value(entity), false };
				event->InvokeUncheckedUnpacked(component, world, entity);
			}
		}
	}
}

void CE::WorldViewportPanel::SetGizmoRect(const glm::vec2 windowPos, glm::vec2 windowSize)
{
	ImGuizmo::SetRect(windowPos.x, windowPos.y, windowSize.x, windowSize.y);
}

bool CE::WorldViewportPanel::GizmoManipulateSelectedTransforms(World& world,
	const std::vector<entt::entity>& selectedEntities,
	const CameraComponent& camera)
{
	Registry& reg = world.GetRegistry();

	const glm::mat4& view = camera.GetView();
	const glm::mat4& proj = camera.GetProjection();

	// The snap needs to be converted to a vec3, otherwise the Y translation does not work.
	const float* snap{};
	if (sShouldGuizmoSnap)
	{
		sCurrentSnapToVec3 = glm::vec3(*sCurrentSnapTo);
		snap = value_ptr(sCurrentSnapToVec3);
	}

	std::vector<TransformComponent*> transformComponents{};
	std::vector<entt::entity> entitiesToTransform{};
	glm::vec3 totalPosition{};
	glm::vec3 totalScale{};

	for (auto entity : selectedEntities)
	{
		auto transformComponent = reg.TryGet<TransformComponent>(entity);

		if (transformComponent != nullptr)
		{
			totalPosition += transformComponent->GetWorldPosition();
			totalScale += transformComponent->GetWorldScale();

			transformComponents.push_back(transformComponent);
			entitiesToTransform.push_back(transformComponent->GetOwner());
		}
	}

	const glm::vec3 avgPosition = totalPosition / static_cast<float>(transformComponents.size());
	const glm::vec3 avgScale = totalScale / static_cast<float>(transformComponents.size());

	glm::mat4 avgMatrix;

	ImGuizmo::RecomposeMatrixFromComponents(value_ptr(avgPosition),
		value_ptr(transformComponents.size() == 1 ? transformComponents[0]->GetWorldOrientationEuler() * (360.0f / TWOPI) : glm::vec3{}),
		value_ptr(avgScale),
		&avgMatrix[0][0]);

	glm::mat4 delta;
	bool isSelected{};

	if (Manipulate(value_ptr(view), value_ptr(proj), sGuizmoOperation, sGuizmoMode, value_ptr(avgMatrix), value_ptr(delta), snap, nullptr, nullptr, &isSelected))
	{
		// Apply the delta to all transformComponents
		for (const auto transformComponent : transformComponents)
		{
			glm::mat4 transformMatrix;

			ImGuizmo::RecomposeMatrixFromComponents(value_ptr(transformComponent->GetWorldPosition()),
				value_ptr(transformComponent->GetWorldOrientationEuler() * (360.0f / TWOPI)),
				value_ptr(transformComponent->GetWorldScale()),
				&transformMatrix[0][0]);

			// TODO Fix the scaling of multiple items at a time
			if (sGuizmoOperation & ImGuizmo::SCALE)
			{
				transformMatrix = transformMatrix * delta;
			}
			else
			{
				transformMatrix = delta * transformMatrix;
			}

			glm::vec3 translation{};
			glm::vec3 eulerOrientation{};
			glm::vec3 scale{};

			ImGuizmo::DecomposeMatrixToComponents(&transformMatrix[0][0], value_ptr(translation), value_ptr(eulerOrientation),
				value_ptr(scale));

			transformComponent->SetWorldMatrix(transformMatrix);
		}
	}

	return isSelected;
}

void CE::WorldDetails::Display(World& world, std::vector<entt::entity>& selectedEntities)
{
	if (selectedEntities.empty())
	{
		ImGui::TextUnformatted("No entities selected");
		return;
	}

	Registry& reg = world.GetRegistry();

	std::vector<std::reference_wrapper<const MetaType>> componentsThatAllSelectedHave{};

	for (auto&& [typeHash, storage] : reg.Storage())
	{
		const MetaType* componentType = MetaManager::Get().TryGetType(typeHash);

		if (componentType == nullptr
			|| componentType->GetProperties().Has(Props::sNoInspectTag))
		{
			continue;
		}

		// Only display components if every selected entity has that component.
		bool allEntitiesHaveOne = true;
		for (entt::entity entity : selectedEntities)
		{
			if (!storage.contains(entity))
			{
				allEntitiesHaveOne = false;
				break;
			}
		}

		if (!allEntitiesHaveOne)
		{
			continue;
		}


		componentsThatAllSelectedHave.emplace_back(*componentType);
	}

	// Most commonly used components are placed at the top
	std::sort(componentsThatAllSelectedHave.begin(), componentsThatAllSelectedHave.end(),
		[&reg](const MetaType& lhs, const MetaType& rhs)
		{
			const size_t lhsCount = reg.Storage(lhs.GetTypeId())->size();
			const size_t rhsCount = reg.Storage(rhs.GetTypeId())->size();

			if (lhsCount != rhsCount)
			{
				return lhsCount > rhsCount;
			}

			return lhs.GetName() > rhs.GetName();
		});

	ImGui::TextUnformatted(NameComponent::GetDisplayName(reg, selectedEntities[0]).c_str());

	if (selectedEntities.size() > 1)
	{
		ImGui::SameLine();
		ImGui::Text(" and %u others", static_cast<uint32>(selectedEntities.size()) - 1);
	}

	const bool addComponentPopUpJustOpened = ImGui::Button(ICON_FA_PLUS);
	ImGui::SetItemTooltip("Add a new component");

	if (addComponentPopUpJustOpened)
	{
		ImGui::OpenPopup("##AddComponentPopUp");
	}

	ImGui::SameLine();

	Search::Begin();

	for (const MetaType& componentClass : componentsThatAllSelectedHave)
	{
		Search::BeginCategory(componentClass.GetName(),
			[&world, &reg, &selectedEntities, &componentClass](std::string_view name)
			{
				bool removeButtonPressed{};

				const bool isHeaderOpen = ImGui::CollapsingHeaderWithButton(name.data(), "X", &removeButtonPressed);

				if (removeButtonPressed)
				{
					for (const auto entity : selectedEntities)
					{
						reg.RemoveComponentIfEntityHasIt(componentClass.GetTypeId(), entity);
					}

					return false;
				}

				if (isHeaderOpen)
				{
					const MetaFunc* const onInspect = TryGetEvent(componentClass, sInspectEvent);

					if (onInspect != nullptr)
					{
						// We run the custom OnInspect here, directly after opening the collapsing header.
						// We unfortunately cannot search through it's contents, so we always show it.
						onInspect->InvokeCheckedUnpacked(world, selectedEntities);
					}
				}

				return isHeaderOpen;
			});

		for (const MetaFunc& func : componentClass.EachFunc())
		{
			if (!func.GetProperties().Has(Props::sCallFromEditorTag))
			{
				continue;
			}

			const bool isMemberFunc = func.GetParameters().size() == 1 && func.GetParameters()[0].mTypeTraits.mStrippedTypeId == componentClass.GetTypeId();

			if (!func.GetParameters().empty()
				&& !isMemberFunc)
			{
				LOG(LogEditor, Warning, "Function {}::{} has {} property, but the function has parameters",
					componentClass.GetName(), func.GetDesignerFriendlyName(), Props::sCallFromEditorTag);
				continue;
			}

			if (Search::AddItem(func.GetDesignerFriendlyName(),
				[&reg, &selectedEntities, &componentClass, &func](std::string_view name)
				{
					// We only do this additional PushId for functions,
					// prevents some weird behaviour
					// occuring if for some ungodly reason
					// a user decided to have a field and function
					// with the same name
					ImGui::PushID(123456789);

					ImGui::PushID(static_cast<int>(componentClass.GetTypeId()));

					const bool wasPressed = ImGui::Button(name.data());

					ImGui::PopID();
					ImGui::PopID();

					return wasPressed;
				}))
			{
				entt::sparse_set* storage = reg.Storage(componentClass.GetTypeId());

				if (storage != nullptr)
				{
					for (const entt::entity entity : selectedEntities)
					{
						if (isMemberFunc)
						{
							MetaAny component{ componentClass, storage->value(entity), false };

							if (component == nullptr)
							{
								LOG(LogEditor, Error, "Error invoking {}::{}: Component was unexpectedly nullptr",
									componentClass.GetName(), func.GetDesignerFriendlyName());
								continue;
							}

							func.InvokeUncheckedUnpacked(component);
						}
						else
						{
							func.InvokeUncheckedUnpacked();
						}
					}
				}
				else
				{
					LOG(LogEditor, Error, "Error invoking {}::{}: Storage was unexpectedly nullptr",
						componentClass.GetName(), func.GetDesignerFriendlyName());
				}
			}
		}


		for (const MetaField& field : componentClass.EachField())
		{
			if (field.GetProperties().Has(Props::sNoInspectTag))
			{
				continue;
			}

			Search::AddItem(field.GetName(),
				[&componentClass, &field, &reg, &selectedEntities](std::string_view fieldName) -> bool
				{
					entt::sparse_set* storage = reg.Storage(componentClass.GetTypeId());

					if (storage == nullptr)
					{
						LOG(LogEditor, Error, "Error inspecting field {}::{}: Storage was unexpectedly nullptr",
							componentClass.GetName(), fieldName);
						return false;
					}

					MetaAny firstComponent{ componentClass, storage->value(selectedEntities[0]), false };

					if (firstComponent == nullptr)
					{
						LOG(LogEditor, Error, "Error inspecting field {}::{}: Component on first entity was unexpectedly nullptr",
							componentClass.GetName(), fieldName);
						return false;
					}

					const MetaType& memberType = field.GetType();

					const TypeTraits constRefMemberType{ memberType.GetTypeId(), TypeForm::ConstRef };
					const FuncId idOfEqualityFunc = MakeFuncId(MakeTypeTraits<bool>(), { constRefMemberType, constRefMemberType });

					const MetaFunc* const equalityOperator = memberType.TryGetFunc(OperatorType::equal, idOfEqualityFunc);

					MetaAny refToValueInFirstComponent = field.MakeRef(firstComponent);

					bool allValuesTheSame = true;

					if (equalityOperator != nullptr)
					{
						for (uint32 i = 1; i < static_cast<uint32>(selectedEntities.size()); i++)
						{
							MetaAny anotherComponent{ componentClass, storage->value(selectedEntities[i]), false };

							if (anotherComponent == nullptr)
							{
								LOG(LogEditor, Error, "Error inspecting field {}::{}: Component was unexpectedly nullptr",
									componentClass.GetName(), fieldName);
								return false;
							}

							MetaAny refToValueInAnotherComponent = field.MakeRef(anotherComponent);

							FuncResult areEqualResult = (*equalityOperator)(refToValueInFirstComponent, refToValueInAnotherComponent);
							ASSERT(!areEqualResult.HasError());
							ASSERT(areEqualResult.HasReturnValue());

							if (!*areEqualResult.GetReturnValue().As<bool>())
							{
								allValuesTheSame = false;
								break;
							}
						}
					}
					else
					{
						LOG(LogEditor, Error, "Missing equality operator for {}::{}. Will assume all the values are the same.",
							field.GetOuterType().GetName(),
							field.GetName());
					}

					if (!allValuesTheSame)
					{
						ImGui::Text("*");
						ImGui::SetItemTooltip("Not all selected entities have the same value.");
						ImGui::SameLine();
					}

					// If values are not the same, just display a zero initialized value.
					FuncResult newValue = allValuesTheSame ? memberType.Construct(refToValueInFirstComponent) : memberType.Construct();

					if (newValue.HasError())
					{
						LOG(LogEditor, Error, "Could not display value for field {}::{} as it could not be default constructed",
							field.GetOuterType().GetName(),
							field.GetName(),
							newValue.Error());
						return false;
					}

					/*
					Makes the variable read-only, it can not be modified through the editor.

					This is implemented by disabling all interaction with the widget. This
					means this may not work for more complex widgets, such as vectors, as
					the user also won't be able to open the collapsing header to view the
					vector.
					*/
					ImGui::BeginDisabled(field.GetProperties().Has(Props::sIsEditorReadOnlyTag));

					const bool wasChanged = ShowInspectUI(std::string{ field.GetName() }, newValue.GetReturnValue());

					ImGui::EndDisabled();

					if (!wasChanged)
					{
						return false;
					}

					for (const entt::entity entity : selectedEntities)
					{
						MetaAny component = reg.Get(componentClass.GetTypeId(), entity);
						MetaAny refToValue = field.MakeRef(component);

						const FuncResult result = memberType.CallFunction(OperatorType::assign, refToValue, newValue.GetReturnValue());

						if (result.HasError())
						{
							LOG(LogEditor, Error, "Updating field value failed, could not copy assign value to {}::{} - {}",
								componentClass.GetName(),
								field.GetName(),
								result.Error());
							return false;
						}
					}

					return true;
				});
		}

		Search::EndCategory({});
	}

	Search::End();

	if (Search::BeginPopup("##AddComponentPopUp"))
	{
		for (const MetaType& type : MetaManager::Get().EachType())
		{
			if (ComponentFilter::IsTypeValid(type)
				&& !type.GetProperties().Has(Props::sNoInspectTag)
				&& std::find_if(componentsThatAllSelectedHave.begin(), componentsThatAllSelectedHave.end(),
					[&type](const MetaType& other)
					{
						return type == other;
					}) == componentsThatAllSelectedHave.end()
				&& Search::Button(type.GetName()))
			{
				for (const entt::entity entity : selectedEntities)
				{
					if (!reg.HasComponent(type.GetTypeId(), entity))
					{
						reg.AddComponent(type, entity);
					}
				}
			}
		}

		Search::EndPopup();
	}
}

static ImVec2 sInvisibleDragDropAreaStart{};

void CE::WorldHierarchy::Display(World& world, std::vector<entt::entity>* selectedEntities)
{
	std::vector<entt::entity> dummySelectedEntities{};
	if (selectedEntities == nullptr)
	{
		selectedEntities = &dummySelectedEntities;
	} // From here on out, we can assume selectedEntities != nullptr

	if (ImGui::IsMouseClicked(1)
		&& ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
	{
		ImGui::OpenPopup("HierarchyPopUp");
	}

	Registry& reg = world.GetRegistry();

	if (ImGui::Button(ICON_FA_PLUS))
	{
		const entt::entity newEntity = reg.Create();

		// Add a transform component, since
		// 99% of entities require it
		reg.AddComponent<TransformComponent>(newEntity);
		reg.AddComponent<NameComponent>(newEntity, "New entity");
	}
	ImGui::SetItemTooltip("Create a new entity");

	ImGui::SameLine();

	Search::Begin(Search::IgnoreParentScore);

	// First we display all entities without transforms
	{
		for (const auto [entity] : reg.Storage<entt::entity>().each())
		{
			if (!reg.HasComponent<TransformComponent>(entity))
			{
				DisplayEntity(reg, entity, *selectedEntities);
			}
		}
	}

	// Now we display only entities with transforms
	{
		for (auto [entity, transform] : reg.View<TransformComponent>().each())
		{
			// We recursively display children.
			// So we only call display if
			// this transform does not have a parent.
			if (transform.IsOrphan())
			{
				DisplayEntity(reg, entity, *selectedEntities);
			}
		}
	}

	Search::End();

	DisplayRightClickPopUp(world, *selectedEntities);

	ImGui::SetCursorScreenPos(sInvisibleDragDropAreaStart);
	ImGui::InvisibleButton("DragToUnparent", glm::max(static_cast<glm::vec2>(ImGui::GetContentRegionAvail()), glm::vec2{ 1.0f }));
	ReceiveDragDropOntoParent(reg, std::nullopt);
	ReceiveDragDrops(world);
}

void CE::WorldHierarchy::DisplayEntity(Registry& registry, entt::entity entity, std::vector<entt::entity>& selectedEntities)
{
	const std::string displayName = NameComponent::GetDisplayName(registry, entity);

	Search::BeginCategory(displayName,
		[&registry, entity, &selectedEntities](std::string_view name) -> bool
		{
			ImGui::PushID(static_cast<int>(entity));

			const TransformComponent* const transform = registry.TryGet<TransformComponent>(entity);

			bool isTreeNodeOpen{};

			if (transform != nullptr
				&& !transform->GetChildren().empty())
			{
				isTreeNodeOpen = ImGui::TreeNode("");
				ImGui::SameLine();
			}

			bool isSelected = std::find(selectedEntities.begin(), selectedEntities.end(), entity) != selectedEntities.end();
			const ImVec2 selectableAreaSize = ImGui::CalcTextSize(name.data(), name.data() + name.size());

			if (ImGui::Selectable(name.data(), &isSelected, 0, selectableAreaSize))
			{
				ToggleIsEntitySelected(selectedEntities, entity);
			}

			ImGui::SetItemTooltip(Format("Entity {}", entt::to_integral(entity)).c_str());

			// Only objects with transforms can accept children
			if (transform != nullptr)
			{
				if (!selectedEntities.empty())
				{
					DragDrop::SendEntities(selectedEntities);
				}

				ReceiveDragDropOntoParent(registry, entity);

				const WeakAssetHandle<Prefab> receivedPrefab = DragDrop::PeekAsset<Prefab>();

				if (receivedPrefab != nullptr
					&& DragDrop::AcceptAsset())
				{
					const entt::entity prefabEntity = registry.CreateFromPrefab( *AssetHandle<Prefab>{ receivedPrefab } );

					TransformComponent* const prefabTransform = registry.TryGet<TransformComponent>(prefabEntity);
					TransformComponent* const parentTransform = registry.TryGet<TransformComponent>(entity);

					if (prefabTransform != nullptr
						&& parentTransform != nullptr)
					{
						prefabTransform->SetParent(parentTransform);
					}
				}
			}

			ImGui::PopID();

			sInvisibleDragDropAreaStart = ImGui::GetCursorScreenPos();

			return isTreeNodeOpen;
		});

	const TransformComponent* const transform = registry.TryGet<TransformComponent>(entity);

	if (transform != nullptr)
	{
		for (const TransformComponent& child : transform->GetChildren())
		{
			DisplayEntity(registry, child.GetOwner(), selectedEntities);
		}
	}

	Search::TreePop();
}

void CE::WorldHierarchy::ReceiveDragDropOntoParent(Registry& registry,
	std::optional<entt::entity> parentAllToThisEntity)
{
	const std::optional<std::vector<entt::entity>> receivedEntities = DragDrop::PeekEntities();

	if (receivedEntities.has_value())
	{
		bool doAllHaveTransforms = true;
		for (const entt::entity entity : *receivedEntities)
		{
			if (registry.TryGet<TransformComponent>(entity) == nullptr)
			{
				doAllHaveTransforms = false;
				break;
			}
		}

		if (doAllHaveTransforms
			&& DragDrop::AcceptEntities())
		{
			TransformComponent* const newParent = parentAllToThisEntity.has_value() ? registry.TryGet<TransformComponent>(*parentAllToThisEntity) : nullptr;

			for (const entt::entity entityToParent : *receivedEntities)
			{
				TransformComponent* childTransform = registry.TryGet<TransformComponent>(entityToParent);

				if (childTransform != nullptr)
				{
					childTransform->SetParent(newParent, true);
				}
			}
		}
	}
}

void CE::WorldHierarchy::DisplayRightClickPopUp(World& world, std::vector<entt::entity>& selectedEntities)
{
	if (!ImGui::BeginPopup("HierarchyPopUp"))
	{
		return;
	}

	if (ImGui::MenuItem("Delete", "Del", nullptr, !selectedEntities.empty()))
	{
		DeleteEntities(world, selectedEntities);
	}

	if (ImGui::MenuItem("Duplicate", "Ctrl+D", nullptr, !selectedEntities.empty()))
	{
		Duplicate(world, selectedEntities);
	}

	if (ImGui::MenuItem("Cut", "Ctrl+X", nullptr, !selectedEntities.empty()))
	{
		CutToClipBoard(world, selectedEntities);
	}

	if (ImGui::MenuItem("Copy", "Ctrl+C", nullptr, !selectedEntities.empty()))
	{
		CopyToClipBoard(world, selectedEntities);
	}

	std::optional<std::string_view> clipboardData = GetSerializedEntitiesInClipboard();
	if (ImGui::MenuItem("Paste", "Ctrl+V", nullptr, clipboardData.has_value()))
	{
		PasteClipBoard(world, selectedEntities, *clipboardData);
	}

	ImGui::EndPopup();
}

namespace
{
	void RemoveInvalidEntities(CE::World& world, std::vector<entt::entity>& selectedEntities)
	{
		selectedEntities.erase(std::remove_if(selectedEntities.begin(), selectedEntities.end(),
			[&world](const entt::entity& entity)
			{
				return !world.GetRegistry().Valid(entity);
			}), selectedEntities.end());
	}

	void ToggleIsEntitySelected(std::vector<entt::entity>& selectedEntities, entt::entity toSelect)
	{
		if (!CE::Input::Get().IsKeyboardKeyHeld(CE::Input::KeyboardKey::LeftControl))
		{
			selectedEntities.clear();
		}

		const auto it = std::find(selectedEntities.begin(), selectedEntities.end(), toSelect);

		if (it == selectedEntities.end())
		{
			selectedEntities.push_back(toSelect);
		}
		else
		{
			selectedEntities.erase(it);
		}
	}

	void DeleteEntities(CE::World& world, std::vector<entt::entity>& selectedEntities)
	{
		world.GetRegistry().Destroy(selectedEntities.begin(), selectedEntities.end(), true);
		selectedEntities.clear();
	}

	// We prepend some form of known string so we can verify whether
	// the clipboard holds copied entities
	constexpr std::string_view sCopiedEntitiesId = "B1C2FF80";

	struct WasRootCopyTag
	{
		static CE::MetaType Reflect()
		{
			CE::MetaType type{ CE::MetaType::T<WasRootCopyTag>{}, "WasRootCopyTag" };
			CE::ReflectComponentType<WasRootCopyTag>(type);
			return type;
		}
	};

	std::string CopyToClipBoard(const CE::World& world, const std::vector<entt::entity>& selectedEntities)
	{
		using namespace CE;

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

	void CutToClipBoard(CE::World& world, std::vector<entt::entity>& selectedEntities)
	{
		CopyToClipBoard(world, selectedEntities);
		DeleteEntities(world, selectedEntities);
	}

	void PasteClipBoard(CE::World& world, std::vector<entt::entity>& selectedEntities, std::string_view clipBoardData)
	{
		if (!IsStringFromCopyToClipBoard(clipBoardData))
		{
			return;
		}

		using namespace CE;

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

	void Duplicate(CE::World& world, std::vector<entt::entity>& selectedEntities)
	{
		const std::string clipboardData = CopyToClipBoard(world, selectedEntities);
		PasteClipBoard(world, selectedEntities, clipboardData);
	}

	std::optional<std::string_view> GetSerializedEntitiesInClipboard()
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

	bool IsStringFromCopyToClipBoard(std::string_view string)
	{
		return string.substr(0, sCopiedEntitiesId.size()) == sCopiedEntitiesId;
	}

	void CheckShortcuts(CE::World& world, std::vector<entt::entity>& selectedEntities)
	{
		using namespace CE;

		RemoveInvalidEntities(world, selectedEntities);

		if (Input::Get().WasKeyboardKeyReleased(Input::KeyboardKey::Delete))
		{
			DeleteEntities(world, selectedEntities);
		}

		if (Input::Get().IsKeyboardKeyHeld(Input::KeyboardKey::LeftControl)
			|| Input::Get().IsKeyboardKeyHeld(Input::KeyboardKey::RightControl))
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

			if (Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::A))
			{
				const auto& entityStorage = world.GetRegistry().Storage<entt::entity>();
				selectedEntities = { entityStorage.begin(), entityStorage.end() };
			}
		}

		if (Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::Escape))
		{
			selectedEntities.clear();
		}

		if (Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::R))
		{
			sGuizmoOperation = ImGuizmo::SCALE;
		}

		if (Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::E))
		{
			sGuizmoOperation = ImGuizmo::ROTATE;
		}

		if (Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::T))
		{
			sGuizmoOperation = ImGuizmo::TRANSLATE;
		}
	}

	void ReceiveDragDrops(CE::World& world)
	{
		using namespace CE;

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
}