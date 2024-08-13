#include "Precomp.h"
#include "Meta/MetaReflect.h"
#include "Meta/ReflectedTypes/ReflectPrimitiveTypes.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectFieldType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"

using namespace CE;
using namespace std;
using T = std::string;

REFLECT_AT_START_UP(ArrayOfT, std::vector<T>);

MetaType Reflector<T>::Reflect()
{
	MetaType typestring{ MetaType::T<T>{}, "String" };

	typestring.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	typestring.AddFunc(std::plus<T>(), OperatorType::add).GetProperties().Add(Props::sIsScriptableTag);
	typestring.AddFunc([](T& n, const T& l) -> T& { n += l; return n; }, OperatorType::add_assign).GetProperties().Add(Props::sIsScriptableTag);
	typestring.AddFunc(std::greater<T>(), OperatorType::greater).GetProperties().Add(Props::sIsScriptableTag);
	typestring.AddFunc(std::less<T>(), OperatorType::less).GetProperties().Add(Props::sIsScriptableTag);
	typestring.AddFunc(std::less<T>(), OperatorType::less).GetProperties().Add(Props::sIsScriptableTag);
	typestring.AddFunc(std::less_equal<T>(), OperatorType::less_equal).GetProperties().Add(Props::sIsScriptableTag);
	typestring.AddFunc(std::equal_to<T>(), OperatorType::equal).GetProperties().Add(Props::sIsScriptableTag);
	typestring.AddFunc(std::not_equal_to<T>(), OperatorType::inequal).GetProperties().Add(Props::sIsScriptableTag);
	typestring.AddFunc([](const T& str) { return std::stoi(str); }, "ToInt32").GetProperties().Add(Props::sIsScriptableTag);
	typestring.AddFunc([](const T& str) { return static_cast<uint32>(std::stoul(str)); }, "ToUInt32").GetProperties().Add(Props::sIsScriptableTag);
	typestring.AddFunc([](const T& str) { return std::stof(str); }, "ToFloat32").GetProperties().Add(Props::sIsScriptableTag);
	typestring.AddFunc([]([[maybe_unused]] const T& str) {
		LOG(LogScripting, Message, "{}", str);
		}, "Print").GetProperties().Add(Props::sIsScriptableTag);

	ReflectFieldType<T>(typestring);
	return typestring;
}
