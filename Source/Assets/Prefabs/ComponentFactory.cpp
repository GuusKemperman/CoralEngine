#include "Precomp.h"
#include "Assets/Prefabs/ComponentFactory.h"

#include "World/Registry.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectFieldType.h"

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

	// As an optimization, we only allow the serialized component to have the custom step as a child
	// This means no lookup time
	//if (!serializedComponent.GetChildren().empty())
	//{
	//	mCustomSerializedData = serializedComponent.GetChildren()[0];
	//}
}

CE::MetaAny CE::ComponentFactory::Construct(Registry& reg, const entt::entity entity) const
{
	MetaAny component = reg.AddComponent(mProductClass.get(), entity); 

	for (const OverridenValue& propValue : mOverridenDefaultValues)
	{
		MetaAny propInComponent = propValue.mField.get().MakeRef(component);

		FuncResult result = propValue.mField.get().GetType().CallFunction(OperatorType::assign, propInComponent, propValue.mValue);

		if (result.HasError())
		{
			LOG(LogAssets, Error, "Could not copy assign {}::{}, prefab will only be partially deserialized - {}",
				propValue.mField.get().GetOuterType().GetName(),
				propValue.mField.get().GetName(),
				result.Error());
		}
	}

	//if (!mCustomSerializedData.IsEmpty())
	//{
	//	// Perfom the custom step
	//	const MetaFunc* const onDeserialize = TryGetComponentOnDeserialize(mProductClass.get());

	//	if (onDeserialize == nullptr)
	//	{
	//		// This component no longer has a custom step,
	//		// so let's ignore it the next time we call this function
	//		LOG(LogAssets, Verbose, "{} no longer has custom OnDeserialize function.",
	//			mProductClass.get().GetName());
	//		mCustomSerializedData.Clear();
	//	}

	//	MetaAny customDataRef = MakeRef(mCustomSerializedData);
	//	MetaAny entityRef = MakeRef(const_cast<entt::entity&>(entity));
	//	MetaAny worldRef = MakeRef(reg.GetWorld());

	//	const FuncResult result = onDeserialize->Invoke({ component, customDataRef, entityRef, worldRef });

	//	if (!result.WasInvoked())
	//	{
	//		LOG(LogAssets, Error, "Failed to invoke custom OnDeserialize step for {}",
	//			mProductClass.get().GetName());
	//		mCustomSerializedData.Clear();
	//	}
	//}

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
