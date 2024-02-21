#include "Precomp.h"
#include "Meta/MetaAny.h"

#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaProps.h"

Engine::MetaAny::MetaAny(const MetaType& type, nullptr_t) :
	MetaAny(type, nullptr, false)
{
}

Engine::MetaAny::MetaAny(const MetaType& type, void* const data, const bool isOwnerOfObject) :
	mTypeInfo(type.GetTypeInfo()),
	mData(data)
{
	mTypeInfo.mFlags |= isOwnerOfObject ? TypeInfo::UserBit : 0;
}

Engine::MetaAny::MetaAny(TypeInfo typeInfo, void* const data, bool isOwnerOfObject) :
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

Engine::MetaAny::MetaAny(MetaAny&& other) noexcept :
	mTypeInfo(other.mTypeInfo),
	mData(other.mData)
{
	other.mData = nullptr;
	other.mTypeInfo.mFlags &= ~TypeInfo::UserBit;
}

Engine::MetaAny::~MetaAny()
{
	DestructAndFree();
}

Engine::MetaAny& Engine::MetaAny::operator=(MetaAny&& other) noexcept
{
	MoveAssign<true>(std::move(other));
	return *this;
}

void Engine::MetaAny::AssignFromAnyOfDifferentType(MetaAny&& other)
{
	MoveAssign<false>(std::move(other));
}

template <bool CheckIfTypesMatch>
void Engine::MetaAny::MoveAssign(MetaAny&& other)
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

bool Engine::MetaAny::IsExactly(const uint32 typeId) const
{
	return mTypeInfo.mTypeId == typeId;
}

const Engine::MetaType* Engine::MetaAny::TryGetType() const
{
	return MetaManager::Get().TryGetType(mTypeInfo.mTypeId);
}

void* Engine::MetaAny::Release()
{
	void* tmp = mData;
	mTypeInfo.mFlags &= ~TypeInfo::UserBit;
	mData = nullptr;
	return tmp;
}

Engine::MetaType Engine::MetaAny::Reflect()
{
	MetaType type = MetaType{ MakeTypeInfo<MetaAny>(), "Any" };
	type.GetProperties().Add(Props::sIsScriptableTag);
	return type;
}

void Engine::MetaAny::DestructAndFree()
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

bool Engine::MetaAny::IsDerivedFrom(const TypeId typeId) const
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
