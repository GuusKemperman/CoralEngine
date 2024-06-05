#include "Precomp.h"
#include "Utilities/Random.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"

uint32 CE::Random::CreateSeed(glm::vec2 position)
{
	glm::vec3 p3 = glm::fract(glm::vec3{ position.x, position.y, position.x } * .1031f);
	p3 += dot(p3, glm::vec3{ p3.y, p3.z, p3.x } + 33.33f);
	const float noise = glm::fract((p3.x + p3.y) * p3.z);
	return static_cast<uint32>(noise * std::numeric_limits<uint32>::max());
}

CE::MetaType CE::Random::Reflect()
{
    MetaType type{ MetaType::T<Random>{}, "Random" };

	type.AddFunc(&Random::Value<int32>, "Int").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&Random::Value<uint32>, "Uint").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&Random::Value<bool>, "Bool").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&Random::Value<float>, "Float").GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc(&Random::Range<int32>, "Int Range", "Min (Inclusive)", "Max (Exclusive)").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&Random::Range<uint32>, "Uint Range", "Min (Inclusive)", "Max (Exclusive)").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&Random::Range<float>, "Float Range", "Min", "Max").GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](glm::vec2 min, glm::vec2 max) { return Range(min, max);  }, "Vec2 Range", MetaFunc::ExplicitParams<glm::vec2, glm::vec2>{}, "Min", "Max").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](glm::vec3 min, glm::vec3 max) { return Range(min, max);  }, "Vec3 Range", MetaFunc::ExplicitParams<glm::vec3, glm::vec3>{}, "Min", "Max").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](glm::vec4 min, glm::vec4 max) { return Range(min, max);  }, "Vec4 Range", MetaFunc::ExplicitParams<glm::vec4, glm::vec4>{}, "Min", "Max").GetProperties().Add(Props::sIsScriptableTag);

    return type;
}
