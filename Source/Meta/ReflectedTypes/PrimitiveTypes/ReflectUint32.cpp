#include "Precomp.h"
#include "Meta/MetaReflect.h"
#include "Meta/ReflectedTypes/ReflectPrimitiveTypes.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectFieldType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"

using namespace CE;
using T = uint32;
REFLECT_AT_START_UP(ArrayOfT, std::vector<T>);

MetaType Reflector<T>::Reflect()
{
	MetaType type{ MetaType::T<T>{}, "Uint" };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::plus<T>(), OperatorType::add).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::minus<T>(), OperatorType::subtract).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::multiplies<T>(), OperatorType::multiplies).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::divides<T>(), OperatorType::divides).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::modulus<T>(), OperatorType::modulus).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::greater<T>(), OperatorType::greater).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::less<T>(), OperatorType::less).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::greater_equal<T>(), OperatorType::greater_equal).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::less_equal<T>(), OperatorType::less_equal).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<T(*)(T, T)>(&glm::max), "Max").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<T(*)(T, T)>(&glm::min), "Min").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<std::string(*)(T)>(&std::to_string), "ToString").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](const T& f) { return static_cast<int32>(f); }, "ToInt32").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](const T& f) { return static_cast<float32>(f); }, "ToFloat32").GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](T& n) -> T& { ++n; return n; }, OperatorType::increment).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](T& n) -> T& { --n; return n; }, OperatorType::decrement).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](T& n, const T& l) -> T& { n += l; return n; }, OperatorType::add_assign).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](T& n, const T& l) -> T& { n -= l; return n; }, OperatorType::subtract_assign).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](T& n, const T& l) -> T& { n *= l; return n; }, OperatorType::multiplies_assign).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](T& n, const T& l) -> T& { n /= l; return n; }, OperatorType::divides_assign).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](T& n, const T& l) -> T& { n %= l; return n; }, OperatorType::modulus_assign).GetProperties().Add(Props::sIsScriptableTag);

	ReflectFieldType<T>(type);

	return type;
}