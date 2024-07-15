#include "Precomp.h"
#include "Utilities/Imgui/WorldViewportPanel.h"

#include <imgui/ImGuizmo.h>
#include <imgui/imgui_internal.h>

#include "Components/CameraComponent.h"
#include "Components/TransformComponent.h"
#include "Core/Input.h"
#include "Rendering/FrameBuffer.h"
#include "Rendering/Renderer.h"
#include "Utilities/Imgui/WorldInspect.h"
#include "World/Registry.h"

namespace CE::Internal
{
	static void CallDrawGizmoEvents(World& world, const std::vector<entt::entity>& selectedEntities);

	static bool GizmoManipulateSelectedTransforms(World& world,
		const std::vector<entt::entity>& selectedEntities,
		const CameraComponent& camera);
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

	Internal::CheckShortcuts(world, *selectedEntities,
		static_cast<Internal::ShortCutType>(
			Internal::ShortCutType::SelectDeselect
			| Internal::ShortCutType::CopyPaste
			| Internal::ShortCutType::Delete
			| Internal::ShortCutType::GuizmoModes));

	Internal::RemoveInvalidEntities(world, *selectedEntities);

	const entt::entity cameraOwner = CameraComponent::GetSelected(world);

	if (cameraOwner == entt::null)
	{
		ImGui::TextUnformatted("No camera");
		return;
	}

	ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
	ImGuizmo::SetRect(windowPos.x + contentMin.x, windowPos.y + contentMin.y, contentSize.x, contentSize.y);

	Renderer::Get().RenderToFrameBuffer(world, frameBuffer, contentSize);

	ImGui::SetCursorPos(contentMin);

	ImGui::Image((ImTextureID)frameBuffer.GetColorTextureId(),
		ImVec2(contentSize),
		ImVec2(0, 0),
		ImVec2(1, 1));

	bool shouldSelectEntityUnderneathMouse = ImGui::IsItemClicked() && ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);

	// Since it is our 'image' that receives the drag drop, we call this right after the image call.
	Internal::ReceiveDragDrops(world);

	ImGui::SetCursorPos(contentMin);

	// There is no need to try to draw gizmos/manipulate transforms when nothing is selected
	if (!selectedEntities->empty()
		&& ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) // ImGuizmo has a global state, so we can only draw one at a time,
		// otherwise translating one object translates it in all open viewports.
	{
		Internal::CallDrawGizmoEvents(world, *selectedEntities);

		// We don't change the selection if we are interacting with the gizmos
		shouldSelectEntityUnderneathMouse &= !Internal::GizmoManipulateSelectedTransforms(world, *selectedEntities, world.GetRegistry().Get<CameraComponent>(cameraOwner));
	}

	if (shouldSelectEntityUnderneathMouse)
	{
		const entt::entity hoveringOver = Internal::GetEntityThatMouseIsHoveringOver(world);

		if (hoveringOver != entt::null)
		{
			Internal::ToggleIsEntitySelected(*selectedEntities, hoveringOver);
		}
		else
		{
			selectedEntities->clear();
		}
	}
}

void CE::Internal::CallDrawGizmoEvents(World& world, const std::vector<entt::entity>& selectedEntities)
{
	Registry& reg = world.GetRegistry();

	for (auto&& [typeHash, storage] : reg.Storage())
	{
		const MetaType* const type = MetaManager::Get().TryGetType(typeHash);

		if (type == nullptr)
		{
			continue;
		}

		const std::optional<BoundEvent> event = TryGetEvent(*type, sOnDrawGizmo);

		if (!event.has_value())
		{
			continue;
		}

		for (entt::entity entity : selectedEntities)
		{
			if (!storage.contains(entity))
			{
				continue;
			}

			if (event->mIsStatic)
			{
				event->mFunc.get().InvokeUncheckedUnpacked(world, entity);
			}
			else
			{
				MetaAny component{ *type, storage.value(entity), false };
				event->mFunc.get().InvokeUncheckedUnpacked(component, world, entity);
			}
		}
	}
}

bool CE::Internal::GizmoManipulateSelectedTransforms(World& world,
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