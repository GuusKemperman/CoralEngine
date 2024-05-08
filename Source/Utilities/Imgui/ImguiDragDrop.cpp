#include "Precomp.h"
#include "Utilities/Imgui/ImguiDragDrop.h"

#include "Assets/Asset.h"
#include "Meta/MetaType.h"

void CE::DragDrop::SendAsset(const Name assetName)
{
	if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
	{
		const Name::HashType nameHash = assetName.GetHash();

		ImGui::SetDragDropPayload("asset", &nameHash, sizeof(Name::HashType));

		ImGui::TextUnformatted(assetName.CString());

		ImGui::EndDragDropSource();
	}
}

CE::WeakAssetHandle<> CE::DragDrop::PeekAsset(TypeId assetTypeId)
{
	if (ImGui::BeginDragDropTarget())
	{
		ImGui::EndDragDropTarget();

		const ImGuiPayload* payload = ImGui::GetDragDropPayload();

		if (payload->IsDataType("asset"))
		{
			ASSERT(payload->DataSize == sizeof(Name::HashType));
			Name::HashType nameHash = *static_cast<Name::HashType*>(payload->Data);

			WeakAssetHandle asset = AssetManager::Get().TryGetWeakAsset(nameHash);

			if (asset != nullptr
				&& asset.GetMetaData().GetClass().IsDerivedFrom(assetTypeId))
			{
				return asset;
			}
			return nullptr;
		}
	}

	return nullptr;
}

bool CE::DragDrop::AcceptAsset()
{
	if (ImGui::BeginDragDropTarget())
	{
		const bool canAccept = ImGui::AcceptDragDropPayload("asset");
		ImGui::EndDragDropTarget();
		return canAccept;
	}
	return false;
}

void CE::DragDrop::SendEntities(const std::vector<entt::entity>& entities)
{
	if (entities.empty())
	{
		return;
	}

	if (ImGui::BeginDragDropSource())
	{
		ImGui::SetDragDropPayload("entities", &entities[0], sizeof(entt::entity) * entities.size());
		ImGui::EndDragDropSource();
	}
}

std::optional<std::vector<entt::entity>> CE::DragDrop::PeekEntities()
{
	if (ImGui::BeginDragDropTarget())
	{
		ImGui::EndDragDropTarget();

		const ImGuiPayload* payload = ImGui::GetDragDropPayload();

		if (payload->IsDataType("entities"))
		{
			const size_t vectorSize = payload->DataSize / sizeof(entt::entity);
			std::vector<entt::entity> entitiesReceived{ vectorSize };
			memcpy(entitiesReceived.data(), static_cast<entt::entity*>(payload->Data), payload->DataSize);
			return entitiesReceived;
		}
	}
	return std::nullopt;
}

bool CE::DragDrop::AcceptEntities()
{
	if (ImGui::BeginDragDropTarget())
	{
		const bool canAccept = ImGui::AcceptDragDropPayload("entities");
		ImGui::EndDragDropTarget();
		return canAccept;
	}
	return false;
}
