#pragma once
#include "MetaTypeIdFwd.h"

namespace CE
{
	class MetaFunc;
	class MetaType;
	class MetaProps;
	class MetaAny;

	/*
	The runtime representation of a field.

	in this struct: struct Vec3 { float x, y, z };, x is a field of Vec3.
	*/
	class MetaField
	{
	public:
		MetaField(const MetaType& outerType,
			const MetaType& fieldType,
			uint32 offset,
			std::string_view name);

		template<typename T, typename Outer>
		MetaField(const MetaType& outer, T Outer::* ptr, std::string_view name);

		MetaField(MetaField&&) noexcept;

		~MetaField();

		bool operator==(const MetaField& other) const;
		bool operator!=(const MetaField& other) const;

		const MetaFunc* GetSetter() const;
		void SetSetter(const MetaFunc* func);

		const MetaFunc* GetGetter() const;
		void SetGetter(const MetaFunc* func);

		std::string_view GetName() const { return mName; }

		const MetaType& GetType() const { return mType; }

		// The type that this field can be found in.
		// TransformComponent is the outertype of TransformComponent::mLocalPosition
		const MetaType& GetOuterType() const { return mOuterType; }

		uintptr GetOffset() const { return mOffset; }
		const MetaProps& GetProperties() const { return *mProperties; }
		MetaProps& GetProperties() { return *mProperties; }

		MetaAny Get(const MetaAny& objectOfOuterType, void* rvoBuffer = nullptr) const;
		void Set(MetaAny& objectOfOuterType, const MetaAny& value) const;

		bool CanGetRef() const;
		bool CanGetConstRef() const;
		MetaAny GetRef(MetaAny& objectOfOuterType) const;
		MetaAny GetConstRef(const MetaAny& objectOfOuterType) const;

	private:
#ifdef ASSERTS_ENABLED
		// Since we can't include MetaType from here, 
		// We use this in the templated constructor to assert 
		// that the provided field is actually from the
		// same type as outer.
		void AssertThatOuterMatches(TypeId expectedTypeId) const;
#endif // ASSERTS_ENABLED

		void* GetFieldAddress(MetaAny& objectOfOuterType) const;
		const void* GetFieldAddress(const MetaAny& objectOfOuterType) const;

		std::reference_wrapper<const MetaType> mType;

		friend class MetaType; // Friends because mOuterType has to be reasigned in MetaType move constructor
		std::reference_wrapper<const MetaType> mOuterType;

		uint32 mOffset{};

		const MetaFunc* mGetter{};
		const MetaFunc* mSetter{};

		std::string mName{};
		std::unique_ptr<MetaProps> mProperties;
	};
}