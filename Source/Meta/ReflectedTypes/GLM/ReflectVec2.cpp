#include "Precomp.h"
#include "Meta/MetaReflect.h"
#include "Meta/ReflectedTypes/ReflectGLM.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectFieldType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"
#include "Utilities/Math.h"

using namespace Engine;
using namespace glm;
using T = vec2;
REFLECT_AT_START_UP(ArrayOfT, std::vector<T>);

MetaType Reflector<T>::Reflect()
{
	MetaType type{ MetaType::T<T>{}, "Vec2" };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::plus<T>(), OperatorType::add, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::minus<T>(), OperatorType::subtract, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::multiplies<T>(), OperatorType::multiplies, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::divides<T>(), OperatorType::divides, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](const T& n) { return -n; }, OperatorType::negate, MetaFunc::ExplicitParams<const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<T(*)(const T&, const T&)>(&max), "Max").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<T(*)(const T&, const T&)>(&min), "Min").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<T(*)(const T&, const T&, const T&)>(&glm::clamp), "Clamp", "Value", "Min", "Max").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&T::x, "X").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&T::y, "Y").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<float(*)(const T&, const T&)>(&dot), "Dot", "DirectionA", "DirectionB").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<T(*)(const T&)>(&normalize), "Normalize").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<float(*)(const T&, const T&)>(&distance), "Distance", "LocationA", "LocationB").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<float(*)(const T&, const T&)>(&distance2), "Distance2", "LocationA", "LocationB").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<float(*)(const T&)>(&length), "Length").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<float(*)(const T&)>(&length2), "Length2").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](const float& x, const float& y) { return T{ x, y }; }, "MakeVec2", MetaFunc::ExplicitParams<const float&, const float&>{}, "X", "Y").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](const T& v, const float& s) { return v * s; }, "Scale", MetaFunc::ExplicitParams<const T&, const float&>{}, "Vector", "Scalar", "Product").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<T(*)(const T&, const float&, const float&)>(&Math::ClampLength), "ClampLength").GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](T& n) -> T& { ++n; return n; }, OperatorType::increment, MetaFunc::ExplicitParams<T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](T& n) -> T& { ++n; return n; }, OperatorType::decrement, MetaFunc::ExplicitParams<T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](T& n, const T& l) -> T& { n += l; return n; }, OperatorType::add_assign, MetaFunc::ExplicitParams<T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](T& n, const T& l) -> T& { n -= l; return n; }, OperatorType::subtract_assign, MetaFunc::ExplicitParams<T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](T& n, const T& l) -> T& { n *= l; return n; }, OperatorType::multiplies_assign, MetaFunc::ExplicitParams<T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](T& n, const T& l) -> T& { n /= l; return n; }, OperatorType::divides_assign, MetaFunc::ExplicitParams<T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);

	ReflectFieldType<T>(type);

	return type;
}