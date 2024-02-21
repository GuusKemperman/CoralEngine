#include "Precomp.h"
#include "Meta/MetaReflect.h"
#include "Meta/ReflectedTypes/ReflectPrimitiveTypes.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectFieldType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"

using namespace Engine;
using T = int32;
REFLECT_AT_START_UP(ArrayOfT, std::vector<T>);


MetaType Reflector<T>::Reflect()
{
	MetaType type{ MetaType::T<T>{}, "Int" };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::plus<T>(), OperatorType::add, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::minus<T>(), OperatorType::subtract, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::multiplies<T>(), OperatorType::multiplies, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::divides<T>(), OperatorType::divides, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::modulus<T>(), OperatorType::modulus, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::greater<T>(), OperatorType::greater, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::less<T>(), OperatorType::less, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::greater_equal<T>(), OperatorType::greater_equal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::less_equal<T>(), OperatorType::less_equal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](const T& n) { return -n; }, OperatorType::negate, MetaFunc::ExplicitParams<const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<T(*)(T, T)>(&glm::max), "Max").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<T(*)(T, T)>(&glm::min), "Min").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<T(*)(T, T, T)>(&glm::clamp), "Clamp", "Value", "Min", "Max").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<std::string(*)(T)>(&std::to_string), "ToString").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](const T& f) { return static_cast<uint32>(f); }, "ToUInt32", MetaFunc::ExplicitParams<const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](const T& f) { return static_cast<float32>(f); }, "ToFloat32", MetaFunc::ExplicitParams<const T&>{}).GetProperties().Add(Props::sIsScriptableTag);

	ReflectFieldType<T>(type);

	return type;
}
