#pragma once
#include "Meta/MetaTypeId.h"

namespace Engine
{
	//****************//
	//		API		  //
	//****************//

	enum class TypeForm
	{
		// Random prime numbers are assigned for better hashing of typetraits

		Value = 3574847,
		Ref = 2062657,
		ConstRef = 51182539,
		Ptr = 71661047,
		ConstPtr = 99105001,
		RValue = 1901583,
	};

	struct TypeTraits
	{
		TypeId mStrippedTypeId{};
		TypeForm mForm{ TypeForm::Value };

		constexpr uint32 Hash() const;

		constexpr auto operator==(const TypeTraits& other) const { return mStrippedTypeId == other.mStrippedTypeId && mForm == other.mForm; }
		constexpr auto operator!=(const TypeTraits& other) const { return mStrippedTypeId != other.mStrippedTypeId || mForm != other.mForm; }
	};
	static_assert(sizeof(TypeTraits) == sizeof(uint64));



	struct TypeInfo
	{
		constexpr TypeInfo() = default;
		constexpr TypeInfo(TypeId typeId, uint32 flags);
		constexpr TypeInfo(
			TypeId typeId,
			uint32 size,
			uint32 alignment,
			bool isTriviallyDefaultConstructible,
			bool isTriviallyMoveConstructible,
			bool isTriviallyCopyConstructible,
			bool isTriviallyCopyAssignable,
			bool isTriviallyMoveAssignable,
			bool isDefaultConstructible,
			bool isMoveConstructible,
			bool isCopyConstructible,
			bool isCopyAssignable,
			bool isMoveAssignable,
			bool isTriviallyDestructible,
			bool userBit = 0);

		static constexpr uint32 sNumOfBitsForSize = 14;	// Max size is 16384
		static constexpr uint32 sNumOfBitsForAlign = 6;	// Max alignment is 64
		static constexpr uint32 sMaxSize = (1 << sNumOfBitsForSize) - 1;
		static constexpr uint32 sMaxAlign = (1 << sNumOfBitsForAlign) - 1;

		static constexpr uint32 sAlignShift = sNumOfBitsForSize;

		enum Masks : uint32
		{
			Size = (1 << sNumOfBitsForSize) - 1,
			Align = ((1 << sNumOfBitsForAlign) - 1) << sAlignShift,
			IsTriviallyDefaultConstructible = 1 << 20,
			IsTriviallyMoveConstructible = 1 << 21,
			IsTriviallyCopyConstructible = 1 << 22,
			IsTriviallyCopyAssignable = 1 << 23,
			IsTriviallyMoveAssignable = 1 << 24,
			IsDefaultConstructible = 1 << 25,
			IsMoveConstructible = 1 << 26,
			IsCopyConstructible = 1 << 27,
			IsCopyAssignable = 1 << 28,
			IsMoveAssignable = 1 << 29,
			IsTriviallyDestructible = 1 << 30,
			UserBit = 1u << 31,
		};
		static_assert((Size & Align) == 0);
		static_assert((Align & IsTriviallyDefaultConstructible) == 0);

		constexpr uint32 GetSize() const { return mFlags & Size; }
		constexpr uint32 GetAlign() const { return (mFlags & Align) >> sAlignShift; }

		TypeId mTypeId{};
		uint32 mFlags{};
	};
	static_assert(sizeof(TypeInfo) == 8);

	constexpr bool CanFormBeNullable(TypeForm form);

	template<typename T>
	CONSTEVAL TypeTraits MakeTypeTraits();

	template<typename T>
	CONSTEVAL TypeInfo MakeTypeInfo();

	template<typename T>
	CONSTEVAL TypeForm MakeTypeForm();

	template<typename T>
	CONSTEVAL uint32 MakeStrippedTypeId();

	//**************************//
	//		Implementation		//
	//**************************//

	constexpr uint32 TypeTraits::Hash() const
	{
		uint64 asUint64 = mStrippedTypeId | (static_cast<uint64>(mForm) << 32);
		return static_cast<uint32>(asUint64 % 4294967291);
	}

	constexpr TypeInfo::TypeInfo(TypeId typeId, uint32 flags) :
		mTypeId(typeId),
		mFlags(flags)
	{
	}

	constexpr TypeInfo::TypeInfo(TypeId typeId, uint32 size, uint32 allignment, bool isTriviallyDefaultConstructible,
	                             bool isTriviallyMoveConstructible, bool isTriviallyCopyConstructible, bool isTriviallyCopyAssignable,
	                             bool isTriviallyMoveAssignable, bool isDefaultConstructible, bool isMoveConstructible,
	                             bool isCopyConstructible, bool isCopyAssignable, bool isMoveAssignable, bool isTriviallyDestructible,
	                             bool userBit) :
		mTypeId(typeId),
		mFlags(size 
		| (allignment << TypeInfo::sAlignShift)
		| (isTriviallyDefaultConstructible  ? IsTriviallyDefaultConstructible : 0)
		| (isTriviallyMoveConstructible  ? IsTriviallyMoveConstructible : 0)
		| (isTriviallyCopyConstructible  ? IsTriviallyCopyConstructible : 0)
		| (isTriviallyCopyAssignable  ? IsTriviallyCopyAssignable : 0)
		| (isTriviallyMoveAssignable  ? IsTriviallyMoveAssignable : 0)
		| (isDefaultConstructible  ? IsDefaultConstructible : 0)
		| (isMoveConstructible  ? IsMoveConstructible : 0)
		| (isCopyConstructible  ? IsCopyConstructible : 0)
		| (isCopyAssignable  ? IsCopyAssignable : 0)
		| (isMoveAssignable  ? IsMoveAssignable : 0)
		| (isTriviallyDestructible  ? IsTriviallyDestructible : 0)
		| (userBit ? UserBit : 0))
	{
		ASSERT(size < sMaxSize);
		ASSERT(allignment < sMaxAlign);
	}

	constexpr bool CanFormBeNullable(TypeForm form)
	{
		return form == TypeForm::Ptr || form == TypeForm::ConstPtr;
	}

	template<typename T>
	CONSTEVAL TypeTraits MakeTypeTraits()
	{
		return { MakeStrippedTypeId<T>(), MakeTypeForm<T>() };
	}

	template <typename TNonStripped>
	CONSTEVAL TypeInfo MakeTypeInfo()
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
	CONSTEVAL TypeForm MakeTypeForm()
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
	CONSTEVAL uint32 MakeStrippedTypeId()
	{
		return MakeTypeId<std::remove_const_t<std::remove_reference_t<std::remove_pointer_t<T>>>>();
	}

	template<>
	struct Engine::EnumStringPairsImpl<TypeForm>
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
}

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
