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
	ImVec2 invisibleDragDropAreaStart{};

	ImGui::SameLine();

	Search::Begin(Search::IgnoreParentScore);

	// Very small chance that registry has not
	// constructed name storage yet. We cant store
	// a reference to transform storage, since it'd be
	// invalidated. Storing the reference is a bit
	// risky, but makes a big impact on performance
	reg.Storage<TransformComponent>();
	entt::storage_for_t<NameComponent>* nameStorage{};
	entt::storage_for_t<TransformComponent>* transformStorage{};
	const auto cacheStorages = [&]
		{
			nameStorage = &reg.Storage<NameComponent>();
			transformStorage = &reg.Storage<TransformComponent>();
		};

	cacheStorages();

	const auto tryGetTransform = [&transformStorage](entt::entity entity)
		{
			return transformStorage->contains(entity) ? &transformStorage->get(entity) : nullptr;
		};

	// We want to reduce the size of the
	// lambda we pass to BeginCategory,
	// so that it fits into the small
	// buffer of std::function so that
	// we avoid a heap allocation for every
	// entity.
	struct
	{
		std::vector<entt::entity>*& mSelectedEntities;
		entt::entity& mClickedEntity;
		ImVec2& mInvisibleDragDropAreaStart;
		Registry& mReg;
		decltype(tryGetTransform)& mTryGetTransform;
		decltype(cacheStorages)& mCacheStorages;

	} context{ selectedEntities, clickedEntity, invisibleDragDropAreaStart, reg, tryGetTransform, cacheStorages };

	const auto displayEntity = [&](const auto& self, entt::entity entity, const TransformComponent* transform) -> void
		{
			const NameComponent* nameComponent = nameStorage->contains(entity) ? &nameStorage->get(entity) : nullptr;
			const std::string_view displayName = NameComponent::GetDisplayName(nameComponent, entity);

			if (Search::BeginCategory(displayName, [entity, transform, &context](std::string_view name) -> bool
				{
					displayOrder.emplace_back(entity);

					ImGui::PushID(static_cast<int>(entity));

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

						const auto shouldForceOpen = std::find_if(context.mSelectedEntities->begin(), context.mSelectedEntities->end(),
							[&](entt::entity selected)
							{
								const TransformComponent* transform = context.mTryGetTransform(selected);
								return transform != nullptr
									&& isChildOfCurrent(isChildOfCurrent, *transform);
							});

						if (shouldForceOpen != context.mSelectedEntities->end())
						{
							ImGui::SetNextItemOpen(true);
						}

						isTreeNodeOpen = ImGui::TreeNode("##");
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

					bool isSelected = std::find(context.mSelectedEntities->begin(),
						context.mSelectedEntities->end(), entity) != context.mSelectedEntities->end();
					const ImVec2 selectableAreaSize = ImGui::CalcTextSize(name.data(), name.data() + name.size());

					if (ImGui::Selectable(name.data(), &isSelected, 0, selectableAreaSize))
					{
						context.mClickedEntity = entity;
					}

					if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
					{
						ImGui::SetTooltip("%s", Format("Entity {}", entt::to_integral(entity)).c_str());
					}

					// Only objects with transforms can accept children
					if (transform != nullptr)
					{
						if (!context.mSelectedEntities->empty())
						{
							DragDrop::SendEntities(*context.mSelectedEntities);
						}

						Internal::ReceiveDragDropOntoParent(context.mReg, entity);

						const WeakAssetHandle<Prefab> receivedPrefab = DragDrop::PeekAsset<Prefab>();

						if (receivedPrefab != nullptr
							&& DragDrop::AcceptAsset())
						{
							const entt::entity prefabEntity = context.mReg.CreateFromPrefab(*AssetHandle<Prefab>{ receivedPrefab });

							// New component types may have been added,
							// invalidating our storage pointers
							context.mCacheStorages();

							TransformComponent* const prefabTransform = context.mTryGetTransform(prefabEntity);
							TransformComponent* const parentTransform = context.mTryGetTransform(entity);

							if (prefabTransform != nullptr
								&& parentTransform != nullptr)
							{
								prefabTransform->SetParent(parentTransform);
							}
						}
					}

					ImGui::PopID();

					context.mInvisibleDragDropAreaStart = ImGui::GetCursorScreenPos();

					return isTreeNodeOpen;
				}))
			{
				if (transform != nullptr)
				{
					for (const TransformComponent& child : transform->GetChildren())
					{
						self(self, child.GetOwner(), &child);
					}
				}

				Search::TreePop();
			}
		};

	{
		for (const auto [entity] : reg.Storage<entt::entity>().each())
		{
			const TransformComponent* transform = tryGetTransform(entity);

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

	ImGui::SetCursorScreenPos(invisibleDragDropAreaStart);
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