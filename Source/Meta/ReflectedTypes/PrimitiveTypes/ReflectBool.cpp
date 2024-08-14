#include "Precomp.h"
#include "Meta/MetaReflect.h"
#include "Meta/ReflectedTypes/ReflectPrimitiveTypes.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectFieldType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"

using namespace CE;
using T = bool;
REFLECT_AT_START_UP(ArrayOfT, std::vector<T>);

MetaType Reflector<T>::Reflect()
{
	MetaType type{ MetaType::T<T>{}, "Bool" };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](const T& b) -> std::string { return b ? "true" : "false"; }, "ToString").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](const T& f) { return static_cast<int32>(f); }, "ToInt32").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](const T& f) { return static_cast<uint32>(f); }, "ToUInt32").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](const T& f) { return static_cast<float32>(f); }, "ToFloat32").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::logical_and<T>(), OperatorType::logical_and).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::logical_or<T>(), OperatorType::logical_or).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::logical_not<T>(), OperatorType::negate).GetProperties().Add(Props::sIsScriptableTag);
	ReflectFieldType<T>(type);

	return type;
}