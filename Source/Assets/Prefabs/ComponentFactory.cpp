#include "Precomp.h"
#include "Assets/Prefabs/ComponentFactory.h"

#include "World/Registry.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"

CE::ComponentFactory::ComponentFactory(const MetaType& objectClass, const BinaryGSONObject& serializedComponent) :
	mProductClass(objectClass)
{
	for (const BinaryGSONMember& serializedProperty : serializedComponent.GetGSONMembers())
	{
		// We serialized the hash of the property's name. Now let's see if its still there
		const MetaField* const prop = objectClass.TryGetField(Name{FromBinary<uint32>(serializedProperty.GetName())});

		if (prop == nullptr)
		{
			LOG(LogAssets, Verbose, "Failed to deserialize property in {} - The property has been removed since we serialized.",
				objectClass.GetName());
			continue;
		}

		if (prop->GetProperties().Has(Props::sNoSerializeTag))
		{
			continue;
		}

		const MetaType& propType = prop->GetType();

		const MetaFunc* const deserializeFunc = propType.TryGetFunc(sDeserializeMemberFuncName);

		if (deserializeFunc == nullptr)
		{
			LOG(LogAssets, Warning, "Failed to deserialize {}::{} - No {} function",
				prop->GetName(),
				objectClass.GetName(),
				sDeserializeMemberFuncName.StringView());
			continue;
		}

		FuncResult defaultConstructResult = propType.Construct();

		if (defaultConstructResult.HasError())
		{
			LOG(LogAssets, Warning, "Failed to deserialize {}::{} - Value of type {} cannot be default constructed", 
				prop->GetName(), 
				objectClass.GetName(),
				propType.GetName());
			continue;
		}

		OverridenValue& overridenValue = mOverridenDefaultValues.emplace_back(*prop, std::move(defaultConstructResult.GetReturnValue()));
		MetaAny refToSerialized{ serializedProperty };

		const FuncResult result = (*deserializeFunc)(serializedProperty, overridenValue.mValue);

		if (result.HasError())
		{
			LOG(LogAssets, Error, "Failed to deserialize {}::{} - {}", 
				prop->GetName(),
				objectClass.GetName(),
				result.Error());
			mOverridenDefaultValues.pop_back();
		}
	}
}

CE::MetaAny CE::ComponentFactory::Construct(Registry& reg, const entt::entity entity) const
{
	MetaAny component = reg.AddComponent(mProductClass.get(), entity); 

	for (const OverridenValue& propValue : mOverridenDefaultValues)
	{
		MetaAny propInComponent = propValue.mField.get().MakeRef(component);

		FuncResult result = propValue.mField.get().GetType().Assign(propInComponent, propValue.mValue);

		if (result.HasError())
		{
			LOG(LogAssets, Error, "Could not copy assign {}::{}, prefab will only be partially deserialized - {}",
				propValue.mField.get().GetOuterType().GetName(),
				propValue.mField.get().GetName(),
				result.Error());
		}
	}

	return component;
}

const CE::MetaAny* CE::ComponentFactory::GetOverridenDefaultValue(const MetaField& prop) const
{
	const auto it = std::find_if(mOverridenDefaultValues.begin(), mOverridenDefaultValues.end(),
	                             [&prop](const OverridenValue& value)
	                             {
		                             return value.mField.get() == prop;
	                             });

	return it == mOverridenDefaultValues.end() ? nullptr : &it->mValue;
}
