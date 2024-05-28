#include "Precomp.h"
#include "Utilities/Imgui/WorldHierarchyPanel.h"

#include "Components/NameComponent.h"
#include "Components/TransformComponent.h"
#include "Core/Input.h"
#include "Utilities/Imgui/ImguiDragDrop.h"
#include "Utilities/Imgui/WorldInspect.h"
#include "World/Registry.h"
#include "Assets/Prefabs/Prefab.h"

namespace CE::Internal
{
	// Nullopt to unparent them
	static void ReceiveDragDropOntoParent(Registry& registry,
		std::optional<entt::entity> parentAllToThisEntity);

	static void DisplayRightClickPopUp(World& world, std::vector<entt::entity>& selectedEntities);
}

void CE::WorldHierarchy::Display(World& world, std::vector<entt::entity>* selectedEntities)
{

	std::vector<entt::entity> dummySelectedEntities{};
	if (selectedEntities == nullptr)
	{
		selectedEntities = &dummySelectedEntities;
	} // From here on out, we can assume selectedEntities != nullptr

	Internal::CheckShortcuts(world, *selectedEntities,
		static_cast<Internal::ShortCutType>(
		Internal::ShortCutType::SelectDeselect 
		| Internal::ShortCutType::CopyPaste
		| Internal::ShortCutType::Delete));

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

	// Reuse the same buffer
	static std::vector<entt::entity> displayOrder{};
	displayOrder.clear();

	entt::entity clickedEntity = entt::null;
	ImVec2 sInvisibleDragDropAreaStart{};

	ImGui::SameLine();

	Search::Begin(Search::IgnoreParentScore);

	const auto displayEntity = [&](const auto& self, entt::entity entity, const TransformComponent* transform) -> void
		{
			const std::string_view displayName = NameComponent::GetDisplayName(reg, entity);

			Search::BeginCategory(displayName,
				[&, entity](std::string_view name) -> bool
				{
					displayOrder.emplace_back(entity);

					ImGui::PushID(static_cast<int>(entity));

					const TransformComponent* const transform = reg.TryGet<TransformComponent>(entity);

					bool isTreeNodeOpen{};

					if (transform != nullptr
						&& !transform->GetChildren().empty())
					{
						const auto isChildOfCurrent = [transform](const auto& self, const TransformComponent& selectedEntityTransform) -> bool
							{
								const TransformComponent* parent = selectedEntityTransform.GetParent();

								if (parent == nullptr)
								{
									return false;
								}

								if (transform == parent)
								{
									return true;
								}

								return self(self, *parent);
							};

						const auto shouldForceOpen = std::find_if(selectedEntities->begin(), selectedEntities->end(),
							[&isChildOfCurrent, &reg](entt::entity selected)
							{
								const TransformComponent* transform = reg.TryGet<TransformComponent>(selected);
								return transform != nullptr
									&& isChildOfCurrent(isChildOfCurrent, *transform);
							});

						if (shouldForceOpen != selectedEntities->end())
						{
							ImGui::SetNextItemOpen(true);
						}

						isTreeNodeOpen = ImGui::TreeNode(Format("##{}", name).data());
						ImGui::SameLine();
					}
					else
					{
						// The three node arrow take up space
						// this makes sure that all entities are
						// aligned, regardless of whether they
						// have children
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 24.0f);
					}

					bool isSelected = std::find(selectedEntities->begin(), selectedEntities->end(), entity) != selectedEntities->end();
					const ImVec2 selectableAreaSize = ImGui::CalcTextSize(name.data(), name.data() + name.size());

					if (ImGui::Selectable(name.data(), &isSelected, 0, selectableAreaSize))
					{
						clickedEntity = entity;
					}

					ImGui::SetItemTooltip(Format("Entity {}", entt::to_integral(entity)).c_str());

					// Only objects with transforms can accept children
					if (transform != nullptr)
					{
						if (!selectedEntities->empty())
						{
							DragDrop::SendEntities(*selectedEntities);
						}

						Internal::ReceiveDragDropOntoParent(reg, entity);

						const WeakAssetHandle<Prefab> receivedPrefab = DragDrop::PeekAsset<Prefab>();

						if (receivedPrefab != nullptr
							&& DragDrop::AcceptAsset())
						{
							const entt::entity prefabEntity = reg.CreateFromPrefab(*AssetHandle<Prefab>{ receivedPrefab });

							TransformComponent* const prefabTransform = reg.TryGet<TransformComponent>(prefabEntity);
							TransformComponent* const parentTransform = reg.TryGet<TransformComponent>(entity);

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

			if (transform != nullptr)
			{
				for (const TransformComponent& child : transform->GetChildren())
				{
					self(self, child.GetOwner(), &child);
				}
			}

			Search::TreePop();
		};

	{
		for (const auto [entity] : reg.Storage<entt::entity>().each())
		{
			const TransformComponent* transform = reg.TryGet<TransformComponent>(entity);

			if (transform == nullptr // We display all entities without transforms
				// We recursively display children.
				// So we only call display if
				// this transform does not have a parent.
				|| transform->IsOrphan())
			{
				displayEntity(displayEntity, entity, transform);
			}
		}
	}

	Search::End();

	Internal::DisplayRightClickPopUp(world, *selectedEntities);

	ImGui::SetCursorScreenPos(sInvisibleDragDropAreaStart);
	ImGui::InvisibleButton("DragToUnparent", glm::max(static_cast<glm::vec2>(ImGui::GetContentRegionAvail()), glm::vec2{ 1.0f }));
	Internal::ReceiveDragDropOntoParent(reg, std::nullopt);
	Internal::ReceiveDragDrops(world);

	if (clickedEntity == entt::null)
	{
		return;
	}

	if (Input::Get().IsKeyboardKeyHeld(CE::Input::KeyboardKey::LeftShift)
		&& !selectedEntities->empty())
	{
		// Select all the entities in between the clicked entity and the entity that was selected before that
		const entt::entity previouslyClickedEntity = selectedEntities->back();

		const auto previousInDisplayOrder = std::find(displayOrder.begin(), displayOrder.end(), previouslyClickedEntity);
		const auto currentInDisplayOrder = std::find(displayOrder.begin(), displayOrder.end(), clickedEntity);

		if (previousInDisplayOrder != displayOrder.end()
			&& currentInDisplayOrder != displayOrder.end())
		{
			const auto begin = currentInDisplayOrder < previousInDisplayOrder ? currentInDisplayOrder : previousInDisplayOrder;
			const auto end = currentInDisplayOrder < previousInDisplayOrder ? previousInDisplayOrder : currentInDisplayOrder;

			// Only select them if they are not already
			for (auto it = begin; it != end; ++it)
			{
				if (std::find(selectedEntities->begin(), selectedEntities->end(), *it) == selectedEntities->end())
				{
					selectedEntities->emplace_back(*it);
				}
			}
		}

		// Ensures the clicked entity is always at the end,
		// so that that is considered the 'start' the next
		// time they shift-click to select
		if (const auto it = std::find(selectedEntities->begin(), selectedEntities->end(), clickedEntity); it != selectedEntities->end())
		{
			selectedEntities->erase(it);
		}
		selectedEntities->emplace_back(clickedEntity);
	}
	else
	{
		Internal::ToggleIsEntitySelected(*selectedEntities, clickedEntity);
	}
}


void CE::Internal::ReceiveDragDropOntoParent(Registry& registry,
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

void CE::Internal::DisplayRightClickPopUp(World& world, std::vector<entt::entity>& selectedEntities)
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