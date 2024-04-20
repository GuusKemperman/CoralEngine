#include "Precomp.h"
#include "Assets/Core/AssetHandle.h"

#include "Core/AssetManager.h"
#include "Meta/Fwd/MetaTypeFwd.h"
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
	CE::WeakAssetHandle weakHandle{ asset };
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

	asset = CE::AssetManager::Get().TryGetWeakAssetHandle(CE::Name{ assetNameHash });

	if (asset == nullptr)
	{
		LOG(LogAssets, Warning, "Asset whose name generated the hash {} no longer exists", assetNameHash);
	}
}

#ifdef EDITOR
void CE::Internal::DisplayHandleWidget(AssetHandle<>& asset, const std::string& name, TypeId type)
{
	WeakAssetHandle weakHandle{ asset };
	DisplayHandleWidget(weakHandle, name, type);
	asset = CE::AssetHandle<>{ weakHandle };
}

void CE::Internal::DisplayHandleWidget(WeakAssetHandle<>& asset, const std::string& name, TypeId type)
{
	if (!Search::BeginCombo(name, asset == nullptr ? "None" : asset.GetMetaData().GetName()))
	{
		return;
	}

	if (Search::Button("None"))
	{
		asset = nullptr;
	}

	for (const WeakAssetHandle<>& weakAsset : AssetManager::Get().GetAllAssetHandles<>())
	{
		if (!weakAsset.GetMetaData().GetClass().IsDerivedFrom(type))
		{
			continue;
		}

		if (Search::Button(weakAsset.GetMetaData().GetName()))
		{
			asset = weakAsset;
		}
	}

	Search::EndCombo();

	WeakAssetHandle<> receivedAsset = DragDrop::PeekAssetHandle(type);

	if (receivedAsset != nullptr
		&& DragDrop::AcceptAsset())
	{
		asset = std::move(receivedAsset);
	}
}
#endif // EDITOR