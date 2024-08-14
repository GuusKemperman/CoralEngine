#include "Precomp.h"
#include "Meta/MetaReflect.h"
#include "Meta/ReflectedTypes/ReflectGLM.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectFieldType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"
#include "Utilities/Math.h"

using namespace CE;
using namespace glm;
using T = vec4;

REFLECT_AT_START_UP(ArrayOfT, std::vector<T>);

MetaType Reflector<T>::Reflect()
{
	MetaType type{ MetaType::T<T>{}, "Vec4" };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::plus<T>(), OperatorType::add).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::minus<T>(), OperatorType::subtract).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::multiplies<T>(), OperatorType::multiplies).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::divides<T>(), OperatorType::divides).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](const T& n) { return -n; }, OperatorType::negate).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<T(*)(const T&, const T&)>(&max), "Max").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<T(*)(const T&, const T&)>(&min), "Min").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<T(*)(const T&, const T&, const T&)>(&glm::clamp), "Clamp", "Value", "Min", "Max").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&T::x, "X").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&T::y, "Y").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&T::z, "Z").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&T::w, "W").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<float(*)(const T&, const T&)>(&dot), "Dot", "DirectionA", "DirectionB").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<T(*)(const T&)>(&normalize), "Normalize").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<T(*)(const T&)>(&radians), "ToRadians", "Degrees", "Radians").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<T(*)(const T&)>(&degrees), "ToDegrees", "Radians", "Degrees").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<float(*)(const T&, const T&)>(&distance), "Distance", "LocationA", "LocationB").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<float(*)(const T&, const T&)>(&distance2), "Distance2", "LocationA", "LocationB").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<float(*)(const T&)>(&length), "Length").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<float(*)(const T&)>(&length2), "Length2").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](const float& x, const float& y, const float& z, const float& w) { return T{ x, y, z, w }; }, "MakeVec4", "X", "Y", "Z", "W").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](const T& v, const float& s) { return v * s; }, "Scale", "Vector", "Scalar", "Product").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(static_cast<T(*)(const T&, const float&, const float&)>(&Math::ClampLength), "ClampLength").GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](T& n) -> T& { ++n; return n; }, OperatorType::increment).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](T& n) -> T& { --n; return n; }, OperatorType::decrement).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](T& n, const T& l) -> T& { n += l; return n; }, OperatorType::add_assign).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](T& n, const T& l) -> T& { n -= l; return n; }, OperatorType::subtract_assign).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](T& n, const T& l) -> T& { n *= l; return n; }, OperatorType::multiplies_assign).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](T& n, const T& l) -> T& { n /= l; return n; }, OperatorType::divides_assign).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const T n) -> std::string
		{
			return "X: " + std::to_string(n.x) + "; Y: " + std::to_string(n.y) + "; Z: " + std::to_string(n.z) + "; W: " + std::to_string(n.w);
		}, "ToString").GetProperties().Add(Props::sIsScriptableTag);

	ReflectFieldType<T>(type);


	return type;
}