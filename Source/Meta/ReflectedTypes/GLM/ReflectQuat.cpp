#include "Precomp.h"
#include "Meta/MetaReflect.h"
#include "Meta/ReflectedTypes/ReflectGLM.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectFieldType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"

using namespace Engine;
using namespace glm;
using T = quat;
REFLECT_AT_START_UP(ArrayOfT, std::vector<T>);

MetaType Reflector<T>::Reflect()
{
	MetaType type{ MetaType::T<T>{}, "Quat" };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::plus<T>(), OperatorType::add, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::minus<T>(), OperatorType::subtract, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::multiplies<T>(), OperatorType::multiplies, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](const T& n) { return -n; }, OperatorType::negate, MetaFunc::ExplicitParams<const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](const glm::vec3& n) { return glm::quat{ n }; }, "EulerToQuat (Radians)", MetaFunc::ExplicitParams<const glm::vec3&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](const glm::quat& n) { return glm::eulerAngles(n); }, "QuatToEuler (Radians)", MetaFunc::ExplicitParams<const glm::quat&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](const glm::quat& n)
	{
		const glm::vec3 direction3D = glm::vec3(0.f, 0.f, 1.f) * n;
		return normalize(glm::vec2(-direction3D.x, direction3D.z));
	}, "QuatToXZDirection (vec2)", MetaFunc::ExplicitParams<const glm::quat&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](const glm::quat& n)
		{
			const glm::vec3 direction3D = glm::vec3(0.f, 0.f, 1.f) * n;
			return normalize(glm::vec3(-direction3D.x, direction3D.y, direction3D.z));
		}, "QuatToDirection (vec3)", MetaFunc::ExplicitParams<const glm::quat&>{}).GetProperties().Add(Props::sIsScriptableTag);

	ReflectFieldType<T>(type);

	return type;
}