#include "Precomp.h"
#include "Components/ComponentFilter.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectFieldType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"

bool CE::Internal::ComponentTypeFilterFunctor::operator()(const MetaType& type) const
{
	return type.GetProperties().Has(Props::sComponentTag);
}

REFLECT_AT_START_UP(ArrayOfComponentFilter, std::vector<CE::ComponentFilter>)

CE::MetaType Reflector<CE::ComponentFilter>::Reflect()
{
	using namespace CE;
	using T = ComponentFilter;

	MetaType type{ MetaType::T<T>{}, "ComponentType" };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);

	ReflectFieldType<T>(type);

	return type;
}
