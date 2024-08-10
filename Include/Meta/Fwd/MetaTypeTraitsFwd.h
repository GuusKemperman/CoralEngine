#pragma once
#include "MetaTypeIdFwd.h"

namespace CE
{
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
		static_assert((Size& Align) == 0);
		static_assert((Align& IsTriviallyDefaultConstructible) == 0);

		constexpr uint32 GetSize() const { return mFlags & Size; }
		constexpr uint32 GetAlign() const { return (mFlags & Align) >> sAlignShift; }

		TypeId mTypeId{};
		uint32 mFlags{};
	};
	static_assert(sizeof(TypeInfo) == 8);

	constexpr bool CanFormBeNullable(TypeForm form);

	template<typename T>
	constexpr TypeTraits MakeTypeTraits();

	template<typename T>
	constexpr TypeInfo MakeTypeInfo();

	template<typename T>
	constexpr TypeForm MakeTypeForm();

	template<typename T>
	constexpr TypeId MakeStrippedTypeId();
}