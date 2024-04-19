#include "Precomp.h"
#include "Assets/Core/AssetHandle.h"

CE::AssetHandleBase::AssetHandleBase() = default;

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
