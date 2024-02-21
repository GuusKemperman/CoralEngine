#pragma once
#include "GSON/GSONBinary.h"

#include "Meta/MetaAny.h"

namespace Engine
{
	class MetaField;
	class Registry;

	// Can be used to construct components using overriden default values. Mostly used by prefab.
	// For most purposes, the Registry::AddComponent function can be used instead.
	class ComponentFactory
	{
	public:
		ComponentFactory(const MetaType& objectClass, const BinaryGSONObject& serializedComponent);

		ComponentFactory(const ComponentFactory&) = delete;
		ComponentFactory(ComponentFactory&&) noexcept = default;;

		ComponentFactory& operator=(ComponentFactory&&) noexcept = default;
		ComponentFactory& operator=(const ComponentFactory&) = delete;


		MetaAny Construct(Registry& registry, entt::entity entity) const;

		const MetaAny* GetOverridenDefaultValue(const MetaField& prop) const;

		const MetaType& GetProductClass() const { return mProductClass; }

		struct OverridenValue
		{
			OverridenValue(const MetaField& field, MetaAny&& value) : mField(field), mValue(std::move(value)) {}
			std::reference_wrapper<const MetaField> mField;
			MetaAny mValue;
		};

		 const std::vector<OverridenValue>& GetOverridenDefaultValues() const { return mOverridenDefaultValues; }
		 //const BinaryGSONObject& GetCustomSerializationData() const { return mCustomSerializedData; }

	private:
		std::reference_wrapper<const MetaType> mProductClass;
		std::vector<OverridenValue> mOverridenDefaultValues{};

		// Will be cleared after the first Construct call, but only if the component no longer
		// has a custom serialization/deserialization step
		//mutable BinaryGSONObject mCustomSerializedData{};
	};
}
