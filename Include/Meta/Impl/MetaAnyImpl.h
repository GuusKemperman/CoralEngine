#pragma once
#include "Meta/Fwd/MetaAnyFwd.h"

#include "Utilities/MemFunctions.h"
#include "Meta/MetaTypeId.h"
#include "Meta/MetaTypeTraits.h"
#include "Meta/MetaReflect.h"

namespace CE::Internal
{
    // Prevents having to include MetaManager.h
    bool DoesTypeExist(TypeId typeId);
}

template <typename T>
CE::MetaAny::MetaAny(T&& anyObject, void* const buffer)
{
    static constexpr TypeTraits traits = MakeTypeTraits<T>();
    static constexpr TypeInfo info = MakeTypeInfo<T>();
    static_assert(traits.mStrippedTypeId != MakeTypeId<MetaAny>(), "There should be other overloads in place to handle cases with MetaAny");
    static_assert(traits.mStrippedTypeId != MakeTypeId<MetaType>(), "You probably meant to use one of the other constructors. Take a look at the other constructors of MetaAny. You will need to provide a boolean as one of the arguments");
    static_assert(traits.mStrippedTypeId != MakeTypeId<TypeTraits>(), "You probably meant to use one of the other constructors. Take a look at the other constructors of MetaAny. You will need to provide a boolean as one of the arguments");

    mTypeInfo = info;

    if constexpr (traits.mForm == TypeForm::Value
        || traits.mForm == TypeForm::RValue)
    {
        mData = buffer;

        if (buffer == nullptr)
        {
            static_assert(sIsReflectable<T>, "Destructor needs to be known, so the type must be reflected");
            mData = FastAlloc(sizeof(T), alignof(T));
            ASSERT(mData != nullptr);

            // We will own the return value; we are responsible for constructing it,
            // and deleting it. This requires knowing the full type.
            ASSERT(Internal::DoesTypeExist(traits.mStrippedTypeId));
            mTypeInfo.mFlags |= TypeInfo::UserBit;
        }

        if constexpr (traits.mForm == TypeForm::Value)
        {
            new(mData)T(anyObject);
        }
        else
        {
            new(mData)T(std::move(anyObject));
        }
    }
    else if constexpr (traits.mForm == TypeForm::Ref)
    {
        mData = &anyObject;
    }
    else if constexpr (traits.mForm == TypeForm::ConstRef)
    {
        using MutableRef = std::remove_const_t<std::remove_reference_t<T>>&;
        mData = &const_cast<MutableRef>(anyObject);
    }
    else if constexpr (traits.mForm == TypeForm::Ptr)
    {
        mData = anyObject;
    }
    else if constexpr (traits.mForm == TypeForm::ConstPtr)
    {
        using MutablePtr = std::remove_const_t<std::remove_pointer_t<T>>*;
        mData = const_cast<MutablePtr>(anyObject);
    }
    else
    {
        static_assert(AlwaysFalse<T>, "TypeForm not handled");
    }
}

template<typename T>
bool CE::MetaAny::IsExactly() const
{
    return IsExactly(MakeTypeId<T>());
}

template<typename T>
T* CE::MetaAny::As()
{
    return IsDerivedFrom<T>() ? static_cast<T*>(mData) : nullptr;
}

template<typename T>
const T* CE::MetaAny::As() const
{
    return IsDerivedFrom<T>() ? static_cast<const T*>(mData) : nullptr;
}

template<typename T>
bool CE::MetaAny::IsDerivedFrom() const
{
    return IsDerivedFrom(MakeTypeId<T>());
}

