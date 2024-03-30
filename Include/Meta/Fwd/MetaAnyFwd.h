#pragma once
#include "MetaTypeIdFwd.h"
#include "MetaTypeTraitsFwd.h"
#include "MetaReflectFwd.h"

namespace CE
{
    class MetaType;

    class MetaAny
    {
    public:
        MetaAny(const MetaType& type, nullptr_t);

        // If isOwnerOfObject is true, this object is responsible for calling the appropriate
        // destructor, and for freeing the buffer.
        MetaAny(const MetaType& type, void* buffer, bool isOwnerOfObject);

        // If isOwnerOfObject is true, this object is responsible for calling the appropriate
        // destructor, and for freeing the buffer. In this, make sure that the type is either
        // trivially destructible, or that the type is reflected, so that the appropriate
        // destructor can be called.
        MetaAny(TypeInfo typeInfo, void* buffer, bool isOwnerOfObject = false);

        /**
         * \brief Constructs a MetaAny referencing the object, or owning the object, depending on how it was passed in.
         * \tparam T
         * \param anyObject If the value was an r-value or moved in, this MetaAny will move-construct the object. Otherwise the MetaAny will be
         * non-owning, and reference the existing object.
         * \param buffer If anyObject is an r-value or is moved in, the anyObject will be move-constructed to the buffer. If no buffer is provided,
         * one will be allocated. This argument is not used if the provided anyObject is not an r-value or moved in.
         */
        template<typename T>
        explicit MetaAny(T&& anyObject, void* buffer = nullptr);

        MetaAny(const MetaAny&) = delete;
        MetaAny(MetaAny&& other) noexcept;

        ~MetaAny();

        MetaAny& operator=(const MetaAny&) = delete;
        MetaAny& operator=(MetaAny&& other) noexcept;

        void AssignFromAnyOfDifferentType(MetaAny&& other);

        bool operator==(nullptr_t) const { return mData == nullptr; }
        bool operator!=(nullptr_t) const { return mData != nullptr; }

        template<typename T>
        bool IsExactly() const;

        bool IsExactly(TypeId typeId) const;

        template<typename T>
        T* As();

        template<typename T>
        const T* As() const;

        const MetaType* TryGetType() const;
        TypeId GetTypeId() const { return mTypeInfo.mTypeId; }
        TypeInfo GetTypeInfo() const { return mTypeInfo; }

        bool IsOwner() const { return mTypeInfo.mFlags & TypeInfo::UserBit; }

        const void* GetData() const { return mData; }
        void* GetData() { return mData; }

        void* Release();

    private:
        friend ReflectAccess;
        static MetaType Reflect();
        REFLECT_AT_START_UP(MetaAny);

        template<bool CheckIfTypesMatch>
        void MoveAssign(MetaAny&& other);

        void DestructAndFree();

        bool IsDerivedFrom(TypeId type) const;

        template<typename T>
        bool IsDerivedFrom() const;

        TypeInfo mTypeInfo{};
        void* mData{};
    };
}