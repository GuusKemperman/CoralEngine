#include "Precomp.h"
#include "Meta/MetaAny.h"

#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaProps.h"

CE::MetaAny::MetaAny(const MetaType& type, nullptr_t) :
	MetaAny(type, nullptr, false)
{
}

CE::MetaAny::MetaAny(const MetaType& type, void* const data, const bool isOwnerOfObject) :
	mTypeInfo(type.GetTypeInfo()),
	mData(data)
{
	mTypeInfo.mFlags |= isOwnerOfObject ? TypeInfo::UserBit : 0;
}

CE::MetaAny::MetaAny(TypeInfo typeInfo, void* const data, bool isOwnerOfObject) :
	mTypeInfo(typeInfo),
	mData(data)
{
	if (isOwnerOfObject)
	{
		mTypeInfo.mFlags |= TypeInfo::UserBit;
	}
	else
	{
		mTypeInfo.mFlags &= ~TypeInfo::UserBit;
	}
}

CE::MetaAny::MetaAny(MetaAny&& other) noexcept :
	mTypeInfo(other.mTypeInfo),
	mData(other.mData)
{
	other.mData = nullptr;
	other.mTypeInfo.mFlags &= ~TypeInfo::UserBit;
}

CE::MetaAny::~MetaAny()
{
	DestructAndFree();
}

CE::MetaAny& CE::MetaAny::operator=(MetaAny&& other) noexcept
{
	MoveAssign<true>(std::move(other));
	return *this;
}

void CE::MetaAny::AssignFromAnyOfDifferentType(MetaAny&& other)
{
	MoveAssign<false>(std::move(other));
}

template <bool CheckIfTypesMatch>
void CE::MetaAny::MoveAssign(MetaAny&& other)
{
	DestructAndFree();

	if constexpr (CheckIfTypesMatch)
	{
		ASSERT(GetTypeId() == other.GetTypeId());
	}
	else
	{
		mTypeInfo = other.mTypeInfo;
	}

	mData = other.mData;
	mTypeInfo.mFlags = other.mTypeInfo.mFlags;

	other.mData = nullptr;
	other.mTypeInfo.mFlags &= ~TypeInfo::UserBit;
}

bool CE::MetaAny::IsExactly(const uint32 typeId) const
{
	return mTypeInfo.mTypeId == typeId;
}

const CE::MetaType* CE::MetaAny::TryGetType() const
{
	return MetaManager::Get().TryGetType(mTypeInfo.mTypeId);
}

void* CE::MetaAny::Release()
{
	void* tmp = mData;
	mTypeInfo.mFlags &= ~TypeInfo::UserBit;
	mData = nullptr;
	return tmp;
}

CE::MetaType CE::MetaAny::Reflect()
{
	MetaType type = MetaType{ MakeTypeInfo<MetaAny>(), "Any" };
	type.GetProperties().Add(Props::sIsScriptableTag);
	return type;
}

void CE::MetaAny::DestructAndFree()
{
	if (mData == nullptr
		|| !IsOwner())
	{
		return;
	}

	if (!(mTypeInfo.mFlags & TypeInfo::IsTriviallyDestructible))
	{
		const MetaType* const type = TryGetType();

		if (type != nullptr)
		{
			type->Destruct(mData, false);
		}
		else
		{
			LOG(LogMeta, Error, "Cannot call destructor for type with id {}: Type was not reflected", GetTypeId());
		}
	}

	MetaType::Free(mData);
}

bool CE::MetaAny::IsDerivedFrom(const TypeId typeId) const
{
	if (IsExactly(typeId))
	{
		return true;
	}

	const MetaType* const type = TryGetType();

	if (type == nullptr)
	{
		LOG(LogMeta, Verbose, "Cannot check if {} is derived from {}, as {} is not reflected", GetTypeId(), typeId, GetTypeId());
		return false;
	}

	return type->IsDerivedFrom(typeId);
}
