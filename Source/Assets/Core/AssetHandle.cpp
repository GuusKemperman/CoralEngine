#include "Precomp.h"
#include "Assets/Core/AssetHandle.h"

#include "Core/AssetManager.h"
#include "Core/Editor.h"
#include "EditorSystems/ThumbnailEditorSystem.h"
#include "Meta/Fwd/MetaTypeFwd.h"
#include "Utilities/NameLookUp.h"
#include "Utilities/Search.h"
#include "Utilities/Imgui/ImguiDragDrop.h"

CE::AssetHandleBase::AssetHandleBase() = default;

CE::AssetHandleBase::AssetHandleBase(nullptr_t)
{
}

CE::AssetHandleBase::AssetHandleBase(AssetHandleBase&& other) noexcept :
	mAssetInternal(other.mAssetInternal)
{
	other.mAssetInternal = nullptr;
}

CE::AssetHandleBase& CE::AssetHandleBase::operator=(AssetHandleBase&& other) noexcept
{
	if (this == &other)
	{
		return *this;
	}

	mAssetInternal = other.mAssetInternal;
	other.mAssetInternal = nullptr;
	return *this;
}

bool CE::AssetHandleBase::operator==(nullptr_t) const
{
	return mAssetInternal == nullptr;
}

bool CE::AssetHandleBase::operator!=(nullptr_t) const
{
	return mAssetInternal != nullptr;
}

bool CE::AssetHandleBase::operator==(const AssetHandleBase& other) const
{
	return mAssetInternal == other.mAssetInternal;
}

bool CE::AssetHandleBase::operator!=(const AssetHandleBase& other) const
{
	return mAssetInternal != other.mAssetInternal;
}

CE::AssetHandleBase::operator bool() const
{
	return mAssetInternal;
}

const CE::AssetFileMetaData& CE::AssetHandleBase::GetMetaData() const
{
	AssureNotNull();
	return mAssetInternal->mMetaData;
}

bool CE::AssetHandleBase::IsLoaded() const
{
	return mAssetInternal != nullptr && mAssetInternal->mAsset != nullptr;
}

const std::optional<std::filesystem::path>& CE::AssetHandleBase::GetFileOfOrigin() const
{
	AssureNotNull();
	return mAssetInternal->mFileOfOrigin;
}

std::vector<std::string> CE::AssetHandleBase::GetOldNames() const
{
	AssureNotNull();

	std::vector<std::string> ret{};
	ret.reserve(mAssetInternal->mOldNames.size());

	for (const std::filesystem::path& pathToRenameFile : mAssetInternal->mOldNames)
	{
		ret.emplace_back(pathToRenameFile.filename().replace_extension().string());
	}

	return ret;
}

uint32 CE::AssetHandleBase::GetNumberOfStrongReferences() const
{
	if (*this == nullptr)
	{
		return 0;
	}

	return mAssetInternal->mRefCounters[static_cast<int>(Internal::AssetInternal::RefCountType::Strong)];
}

uint32 CE::AssetHandleBase::GetNumberOfSoftReferences() const
{
	if (*this == nullptr)
	{
		return 0;
	}

	return mAssetInternal->mRefCounters[static_cast<int>(Internal::AssetInternal::RefCountType::Weak)];
}

const CE::Asset* CE::AssetHandleBase::GetAssetBase() const
{
	if (mAssetInternal == nullptr)
	{
		return nullptr;
	}

	mAssetInternal->mHasBeenDereferencedSinceGarbageCollect = true;
	return mAssetInternal->TryGetLoadedAsset();
}

void CE::AssetHandleBase::AssureNotNull() const
{
	ASSERT_LOG(*this != nullptr, "Attempted to dereference null handle");
}

bool CE::AssetHandleBase::IsA(TypeId type) const
{
	return mAssetInternal != nullptr
		&& GetMetaData().GetClass().IsDerivedFrom(type);
}

void cereal::save(BinaryOutputArchive& archive, const CE::AssetHandleBase& asset)
{
	if (asset != nullptr)
	{
		const CE::Name::HashType hash = CE::Name::HashString(asset.GetMetaData().GetName());
		archive(hash);
	}
	else
	{
		archive(CE::Name::HashType{});
	}
}

void cereal::load(BinaryInputArchive& archive, CE::AssetHandle<>& asset)
{
	CE::WeakAssetHandle<> weakHandle{ asset };
	load(archive, weakHandle);
	asset = CE::AssetHandle<>{ weakHandle };
}

void cereal::load(BinaryInputArchive& archive, CE::WeakAssetHandle<>& asset)
{
	CE::Name::HashType assetNameHash{};
	archive(assetNameHash);

	if (assetNameHash == 0)
	{
		asset = nullptr;
		return;
	}

	asset = CE::AssetManager::Get().TryGetWeakAsset(CE::Name{ assetNameHash });

	if (asset != nullptr)
	{
		return;
	}

#ifdef LOGGING_ENABLED
	CE::sNameLookUpMutex.lock();
	const auto it = CE::sNameLookUp.find(assetNameHash);
	const std::string_view name = it == CE::sNameLookUp.end() ? std::string_view{ "(Unknown name)" } : std::string_view{ it->second };

	LOG(LogAssets, Warning, "Asset {} (hash {}) no longer exists", name, assetNameHash);
	CE::sNameLookUpMutex.unlock();
#endif
}

#ifdef EDITOR
void CE::Internal::DisplayHandleWidget(AssetHandle<>& asset, const std::string& name, TypeId type)
{
	WeakAssetHandle<> weakHandle{ asset };
	DisplayHandleWidget(weakHandle, name, type);
	asset = CE::AssetHandle<>{ weakHandle };
}

void CE::Internal::DisplayHandleWidget(WeakAssetHandle<>& asset, const std::string& name, TypeId type)
{
	ThumbnailEditorSystem* thumbnailEditorSystem = Editor::Get().TryGetSystem<ThumbnailEditorSystem>();

	if (thumbnailEditorSystem == nullptr)
	{
		LOG(LogEditor, Error, "Could not display content, no thumbnail editor system!");
	}

	const bool isOpen = Search::BeginCombo(name, asset == nullptr ? "None" : asset.GetMetaData().GetName());

	WeakAssetHandle<> receivedAsset = DragDrop::PeekAsset(type);

	if (receivedAsset != nullptr
		&& DragDrop::AcceptAsset())
	{
		asset = std::move(receivedAsset);
	}

	if (!isOpen)
	{
		if (asset != nullptr)
		{
			ImGui::SameLine();
			const glm::vec2 thumbnailSize{ 32.0f };

			ImGui::Image(thumbnailEditorSystem->GetThumbnail(asset), thumbnailSize);

			if (Editor::Get().IsThereAnEditorTypeForAssetType(type))
			{
				ImGui::SameLine();
				if (ImGui::Button(ICON_FA_PENCIL))
				{
					Editor::Get().TryOpenAssetForEdit(asset);
				}
				ImGui::SetItemTooltip("Open this asset for edit");
			}
		}

		return;
	}

	if (Search::Button("None"))
	{
		asset = nullptr;
	}

	for (const WeakAssetHandle<>& weakAsset : AssetManager::Get().GetAllAssets<>())
	{
		if (!weakAsset.GetMetaData().GetClass().IsDerivedFrom(type))
		{
			continue;
		}

		if (Search::AddItem(weakAsset.GetMetaData().GetName(),
			[weakAsset, thumbnailEditorSystem](std::string_view name)
			{
				const glm::vec2 thumbnailSize{ ImGui::GetTextLineHeightWithSpacing() };
				const bool imagePressed = thumbnailEditorSystem->DisplayImGuiImageButton(weakAsset, thumbnailSize);
				ImGui::SameLine();
				const bool menuItemPressed = ImGui::MenuItem(name.data());
				return menuItemPressed || imagePressed;
			}))
		{
			asset = weakAsset;
		}
	}

	Search::EndCombo();
}
#endif // EDITOR