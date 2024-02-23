#include "Precomp.h"
#include "Meta/MetaReflect.h"
#include "Meta/ReflectedTypes/ReflectPrimitiveTypes.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectFieldType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"

using namespace Engine;
using namespace std;
using T = std::string;

REFLECT_AT_START_UP(ArrayOfT, std::vector<T>);

MetaType Reflector<T>::Reflect()
{
	MetaType typestring{ MetaType::T<T>{}, "String" };

	typestring.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	typestring.AddFunc(std::plus<T>(), OperatorType::add, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	typestring.AddFunc(std::greater<T>(), OperatorType::greater, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	typestring.AddFunc(std::less<T>(), OperatorType::less, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	typestring.AddFunc(std::less<T>(), OperatorType::less, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	typestring.AddFunc(std::less_equal<T>(), OperatorType::less_equal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	typestring.AddFunc(std::equal_to<T>(), OperatorType::equal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	typestring.AddFunc(std::not_equal_to<T>(), OperatorType::inequal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	typestring.AddFunc([](const T& str) { return std::stoi(str); }, "ToInt32", MetaFunc::ExplicitParams<const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	typestring.AddFunc([](const T& str) { return static_cast<uint32>(std::stoul(str)); }, "ToUInt32", MetaFunc::ExplicitParams<const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	typestring.AddFunc([](const T& str) { return std::stof(str); }, "ToFloat32", MetaFunc::ExplicitParams<const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	typestring.AddFunc([]([[maybe_unused]] const T& str) {
		LOG_FMT(LogScripting, Message, "{}", str);
		}, "Print", MetaFunc::ExplicitParams<const T&>{}).GetProperties().Add(Props::sIsScriptableTag);

	ReflectFieldType<T>(typestring);
	return typestring;
}
