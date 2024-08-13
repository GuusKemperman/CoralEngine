#include "Precomp.h"
#include "Meta/MetaReflect.h"
#include "Meta/ReflectedTypes/ReflectGLM.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectFieldType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"

using namespace CE;
using namespace glm;
using T = mat4;

REFLECT_AT_START_UP(ArrayOfT, std::vector<T>);

MetaType Reflector<T>::Reflect()
{
	MetaType type{ MetaType::T<T>{}, "Mat4" };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::multiplies<T>(), OperatorType::multiplies).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](const T& n) { return -n; }, OperatorType::negate).GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](T& n, const T& l) -> T& { n *= l; return n; }, OperatorType::multiplies_assign).GetProperties().Add(Props::sIsScriptableTag);

	ReflectFieldType<T>(type);

	return type;
}
