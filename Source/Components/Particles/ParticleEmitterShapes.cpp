#include "Precomp.h"
#include "Components/Particles/ParticleEmitterShapes.h"

#include "Components/Particles/ParticleEmitterComponent.h"
#include "Components/Particles/ParticleUtilities.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Math.h"
#include "Utilities/Random.h"

namespace CE::Internal
{
	static void RandomOrientation(ParticleEmitterComponent& emitter, uint32 i, glm::vec3 minOrientation, glm::vec3 maxOrientation, glm::quat emitterWorldOrientation);
}

void CE::ParticleEmitterShapeAABB::OnParticleSpawn(ParticleEmitterComponent& emitter,
	const uint32 i,
	const glm::quat emitterWorldOrientation,
	const glm::mat4& emitterMatrix) const
{
	emitter.SetParticlePositionFast(i, emitterMatrix * glm::vec4{ Random::Range(mMinPosition, mMaxPosition), 1.0f });
	Internal::RandomOrientation(emitter, i, mMinOrientation, mMinOrientation, emitterWorldOrientation);
}

void CE::ParticleEmitterShapeSphere::OnParticleSpawn(ParticleEmitterComponent& emitter, uint32 i,
	glm::quat emitterWorldOrientation, [[maybe_unused]] const glm::mat4& emitterMatrix) const
{
	const float u = Random::Value<float>();
	const float v = Random::Value<float>();
	const float r = std::cbrtf(Random::Value<float>());
	const float theta = u * 2.0f * TWOPI;
	const float phi = glm::acos(2.0f * v - 1.0f);
	const float sinTheta = glm::sin(theta);
	const float cosTheta = glm::cos(theta);
	const float sinPhi = glm::sin(phi);
	const float cosPhi = glm::cos(phi);
	const float x = mRadius * r * sinPhi * cosTheta;
	const float y = mRadius * r * sinPhi * sinTheta;
	const float z = mRadius * r * cosPhi;

	emitter.SetParticlePositionFast(i, emitterMatrix * glm::vec4{ x, y, z, 1.0f });
	Internal::RandomOrientation(emitter, i, mMinOrientation, mMinOrientation, emitterWorldOrientation);
}

void CE::Internal::RandomOrientation(ParticleEmitterComponent& emitter, uint32 i, glm::vec3 minOrientation, glm::vec3 maxOrientation, glm::quat emitterWorldOrientation)
{
	emitter.SetParticleOrientationFast(i, emitterWorldOrientation * glm::quat{ glm::radians(Random::Range(minOrientation, maxOrientation)) });
}

CE::MetaType CE::ParticleEmitterShapeAABB::Reflect()
{
	MetaType type = MetaType{ MetaType::T<ParticleEmitterShapeAABB>{}, "ParticleEmitterShapeAABB" };
	type.GetProperties().Add(Props::sIsScriptableTag);

	type.AddField(&ParticleEmitterShapeAABB::mMinPosition, "mMinPosition").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticleEmitterShapeAABB::mMaxPosition, "mMaxPosition").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticleEmitterShapeAABB::mMinOrientation, "mMinOrientation").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticleEmitterShapeAABB::mMaxOrientation, "mMaxOrientation").GetProperties().Add(Props::sIsScriptableTag);

	ReflectParticleComponentType<ParticleEmitterShapeAABB>(type);
	return type;
}

CE::MetaType CE::ParticleEmitterShapeSphere::Reflect()
{
	MetaType type = MetaType{ MetaType::T<ParticleEmitterShapeSphere>{}, "ParticleEmitterShapeSphere" };
	type.GetProperties().Add(Props::sIsScriptableTag);

	type.AddField(&ParticleEmitterShapeSphere::mRadius, "mRadius").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticleEmitterShapeSphere::mMinOrientation, "mMinOrientation").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticleEmitterShapeSphere::mMaxOrientation, "mMaxOrientation").GetProperties().Add(Props::sIsScriptableTag);

	ReflectParticleComponentType<ParticleEmitterShapeSphere>(type);
	return type;
}

