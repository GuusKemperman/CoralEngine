#pragma once
#include "magic_enum/magic_enum.hpp"

#include "Meta/Fwd/MetaTypeTraitsFwd.h"
#include "Meta/MetaTypeId.h"

constexpr uint32 CE::TypeTraits::Hash() const
{
	uint64 formHash;
	switch (mForm)
	{
		case TypeForm::Value:formHash = 3574847;break;
		case TypeForm::Ref:formHash = 2062657;break;
		case TypeForm::ConstRef:formHash = 51182539;break;
		case TypeForm::Ptr:formHash = 71661047;break;
		case TypeForm::ConstPtr:formHash = 99105001;break;
		case TypeForm::RValue:formHash = 1901583; break;
		default: formHash = {};
	}

	uint64 asUint64 = mStrippedTypeId | (formHash << 32);
	return static_cast<uint32>(asUint64 % 4294967291);
}

constexpr CE::TypeInfo::TypeInfo(TypeId typeId, uint32 flags) :
	mTypeId(typeId),
	mFlags(flags)
{
}

constexpr CE::TypeInfo::TypeInfo(TypeId typeId, uint32 size, uint32 allignment, bool isTriviallyDefaultConstructible,
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
}

constexpr bool CE::CanFormBeNullable(TypeForm form)
{
	return form == TypeForm::Ptr || form == TypeForm::ConstPtr;
}

template<typename T>
CONSTEVAL CE::TypeTraits CE::MakeTypeTraits()
{
	return { MakeStrippedTypeId<T>(), MakeTypeForm<T>() };
}

template <typename TNonStripped>
CONSTEVAL CE::TypeInfo CE::MakeTypeInfo()
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
CONSTEVAL CE::TypeForm CE::MakeTypeForm()
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
CONSTEVAL CE::TypeId CE::MakeStrippedTypeId()
{
	return MakeTypeId<std::remove_const_t<std::remove_reference_t<std::remove_pointer_t<T>>>>();
}

#include "cereal/archives/binary.hpp"

namespace cereal
{
	class BinaryOutputArchive;
	class BinaryInputArchive;

	inline void save(BinaryOutputArchive& ar, const CE::TypeForm& form)
	{
		uint32 typeForm = static_cast<uint32>(form);
		ar(typeForm);
	}

	inline void load(BinaryInputArchive& ar, CE::TypeForm& form)
	{
		uint32 typeForm{};
		ar(typeForm);

		// Backwards compatibility
		switch (typeForm)
		{
		case 3574847: typeForm = static_cast<uint32>(CE::TypeForm::Value); break;
		case 2062657: typeForm = static_cast<uint32>(CE::TypeForm::Ref); break;
		case 51182539: typeForm = static_cast<uint32>(CE::TypeForm::ConstRef); break;
		case 71661047: typeForm = static_cast<uint32>(CE::TypeForm::Ptr); break;
		case 99105001: typeForm = static_cast<uint32>(CE::TypeForm::ConstPtr); break;
		case 1901583: typeForm = static_cast<uint32>(CE::TypeForm::RValue); break;
		default:;
		}
		form = static_cast<CE::TypeForm>(typeForm);
	}

	inline void save(BinaryOutputArchive& ar, const CE::TypeTraits& traits)
	{
		ar(traits.mStrippedTypeId);
		ar(traits.mForm);
	}

	inline void load(BinaryInputArchive& ar, CE::TypeTraits& traits)
	{
		ar(traits.mStrippedTypeId);
		ar(traits.mForm);
	}
}

CEREAL_SPECIALIZE_FOR_ALL_ARCHIVES(CE::TypeForm, cereal::specialization::non_member_load_save);
