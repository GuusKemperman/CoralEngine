#pragma once
#include "Meta/Fwd/MetaTypeTraitsFwd.h"

#include "Meta/MetaTypeId.h"

constexpr uint32 Engine::TypeTraits::Hash() const
{
	uint64 asUint64 = mStrippedTypeId | (static_cast<uint64>(mForm) << 32);
	return static_cast<uint32>(asUint64 % 4294967291);
}

constexpr Engine::TypeInfo::TypeInfo(TypeId typeId, uint32 flags) :
	mTypeId(typeId),
	mFlags(flags)
{
}

constexpr Engine::TypeInfo::TypeInfo(TypeId typeId, uint32 size, uint32 allignment, bool isTriviallyDefaultConstructible,
	bool isTriviallyMoveConstructible, bool isTriviallyCopyConstructible, bool isTriviallyCopyAssignable,
	bool isTriviallyMoveAssignable, bool isDefaultConstructible, bool isMoveConstructible,
	bool isCopyConstructible, bool isCopyAssignable, bool isMoveAssignable, bool isTriviallyDestructible,
	bool userBit) :
	mTypeId(typeId),
	mFlags(size
		| (allignment << TypeInfo::sAlignShift)
		| (isTriviallyDefaultConstructible ? IsTriviallyDefaultConstructible : 0)
		| (isTriviallyMoveConstructible ? IsTriviallyMoveConstructible : 0)
		| (isTriviallyCopyConstructible ? IsTriviallyCopyConstructible : 0)
		| (isTriviallyCopyAssignable ? IsTriviallyCopyAssignable : 0)
		| (isTriviallyMoveAssignable ? IsTriviallyMoveAssignable : 0)
		| (isDefaultConstructible ? IsDefaultConstructible : 0)
		| (isMoveConstructible ? IsMoveConstructible : 0)
		| (isCopyConstructible ? IsCopyConstructible : 0)
		| (isCopyAssignable ? IsCopyAssignable : 0)
		| (isMoveAssignable ? IsMoveAssignable : 0)
		| (isTriviallyDestructible ? IsTriviallyDestructible : 0)
		| (userBit ? UserBit : 0))
{
	ASSERT(size < sMaxSize);
	ASSERT(allignment < sMaxAlign);
}

constexpr bool Engine::CanFormBeNullable(TypeForm form)
{
	return form == TypeForm::Ptr || form == TypeForm::ConstPtr;
}

template<typename T>
CONSTEVAL Engine::TypeTraits Engine::MakeTypeTraits()
{
	return { MakeStrippedTypeId<T>(), MakeTypeForm<T>() };
}

template <typename TNonStripped>
CONSTEVAL Engine::TypeInfo Engine::MakeTypeInfo()
{
	using StrippedT = std::remove_const_t<std::remove_reference_t<std::remove_pointer_t<TNonStripped>>>;

	uint32 size{};
	uint32 align{};

	if constexpr (!std::is_same_v<StrippedT, void>)
	{
		static_assert(sizeof(StrippedT) < TypeInfo::sMaxSize, "Class size is too big");
		static_assert(alignof(StrippedT) < TypeInfo::sMaxAlign, "Class alignment is too high");

		size = sizeof(StrippedT);
		align = alignof(StrippedT);
	}

	TypeInfo typeInfo{
		MakeStrippedTypeId<TNonStripped>(),
		size,
		align,
		std::is_trivially_default_constructible_v<StrippedT>,
		std::is_trivially_move_constructible_v<StrippedT>,
		std::is_trivially_copy_constructible_v<StrippedT>,
		std::is_trivially_copy_assignable_v<StrippedT>,
		std::is_trivially_move_assignable_v<StrippedT>,
		std::is_default_constructible_v<StrippedT>,
		std::is_move_constructible_v<StrippedT>,
		std::is_copy_constructible_v<StrippedT>,
		std::is_copy_assignable_v<StrippedT>,
		std::is_move_assignable_v<StrippedT>,
		std::is_trivially_destructible_v<StrippedT>
	};

	return typeInfo;
}

template <typename T>
CONSTEVAL Engine::TypeForm Engine::MakeTypeForm()
{
	if constexpr (std::is_rvalue_reference_v<T>)
	{
		static_assert(!std::is_pointer_v<std::remove_reference_t<T>>, "References to pointers are not allowed");
		return TypeForm::RValue;
	}
	else  if constexpr (std::is_reference_v<T>)
	{
		static_assert(!std::is_pointer_v<std::remove_reference_t<T>>, "References to pointers are not allowed");
		return std::is_const_v<std::remove_reference_t<T>> ? TypeForm::ConstRef : TypeForm::Ref;
	}
	else if constexpr (std::is_pointer_v<T>)
	{
		static_assert(!std::is_pointer_v<std::remove_pointer_t<T>>, "Pointers to pointers are not allowed");
		return std::is_const_v<std::remove_pointer_t<T>> ? TypeForm::ConstPtr : TypeForm::Ptr;
	}
	else
	{
		return TypeForm::Value;
	}
}

template <typename T>
CONSTEVAL Engine::TypeId Engine::MakeStrippedTypeId()
{
	return MakeTypeId<std::remove_const_t<std::remove_reference_t<std::remove_pointer_t<T>>>>();
}

template<>
struct Engine::EnumStringPairsImpl<Engine::TypeForm>
{
	static constexpr EnumStringPairs<TypeForm, 137> value = {
		EnumStringPair<TypeForm> { TypeForm::Value, "Value" },
		{ TypeForm::Ref, "Ref" },
		{ TypeForm::ConstRef, "ConstRef" },
		{ TypeForm::Ptr, "Ptr" },
		{ TypeForm::ConstPtr, "ConstPtr" },
		{ TypeForm::RValue, "RValue" }
	};
};

#include "cereal/archives/binary.hpp"

namespace cereal
{
	class BinaryOutputArchive;
	class BinaryInputArchive;

	inline void save(BinaryOutputArchive& ar, const Engine::TypeTraits& traits)
	{
		ar.saveBinary(&traits, sizeof(traits));
	}

	inline void load(BinaryInputArchive& ar, Engine::TypeTraits& traits)
	{
		ar.loadBinary(&traits, sizeof(traits));
	}
}