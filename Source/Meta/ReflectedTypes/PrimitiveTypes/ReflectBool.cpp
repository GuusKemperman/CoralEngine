#include "Precomp.h"
#include "Meta/MetaReflect.h"
#include "Meta/ReflectedTypes/ReflectPrimitiveTypes.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectFieldType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"

using namespace Engine;
using T = bool;
REFLECT_AT_START_UP(ArrayOfT, std::vector<T>);

MetaType Reflector<T>::Reflect()
{
	MetaType type{ MetaType::T<T>{}, "Bool" };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](const T& b) -> std::string { return b ? "true" : "false"; }, "ToString", MetaFunc::ExplicitParams<const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](const T& f) { return static_cast<int32>(f); }, "ToInt32", MetaFunc::ExplicitParams <const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](const T& f) { return static_cast<uint32>(f); }, "ToUInt32", MetaFunc::ExplicitParams<const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](const T& f) { return static_cast<float32>(f); }, "ToFloat32", MetaFunc::ExplicitParams<const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::logical_and<T>(), OperatorType::logical_and, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::logical_or<T>(), OperatorType::logical_or, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::logical_not<T>(), OperatorType::negate, MetaFunc::ExplicitParams<const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	ReflectFieldType<T>(type);

	return type;
}