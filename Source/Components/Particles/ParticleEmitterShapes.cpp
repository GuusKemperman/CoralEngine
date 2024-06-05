#include "Precomp.h"
#include "Components/Particles/ParticleEmitterShapes.h"

#include "Components/Particles/ParticleEmitterComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Math.h"
#include "Utilities/Random.h"
#include "Utilities/Reflect/ReflectComponentType.h"

namespace CE::Internal
{
	static void RandomScale(ParticleEmitterComponent& emitter, size_t i, glm::vec3 minScale, glm::vec3 maxScale, glm::vec3 emitterWorldScale);
	static void RandomOrientation(ParticleEmitterComponent& emitter, size_t i, glm::vec3 minOrientation, glm::vec3 maxOrientation, glm::quat emitterWorldOrientation);
}

void CE::ParticleEmitterShapeAABB::OnParticleSpawn(ParticleEmitterComponent& emitter,
	const size_t i,
	const glm::quat emitterWorldOrientation,
	const glm::vec3 emitterWorldScale,
	const glm::mat4& emitterMatrix) const
{
	emitter.GetParticlePositions()[i] = emitterMatrix * glm::vec4{ Random::Range(mMinPosition, mMaxPosition), 1.0f };
	Internal::RandomScale(emitter, i, mMinScale, mMaxScale, emitterWorldScale);
	Internal::RandomOrientation(emitter, i, mMinOrientation, mMinOrientation, emitterWorldOrientation);
}

void CE::ParticleEmitterShapeSphere::OnParticleSpawn(ParticleEmitterComponent& emitter, size_t i,
	glm::quat emitterWorldOrientation, glm::vec3 emitterWorldScale, [[maybe_unused]] const glm::mat4& emitterMatrix) const
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
	const float x = r * sinPhi * cosTheta;
	const float y = r * sinPhi * sinTheta;
	const float z = r * cosPhi;

	emitter.GetParticlePositions()[i] = { x, y, z };
	Internal::RandomScale(emitter, i, mMinScale, mMaxScale, emitterWorldScale);
	Internal::RandomOrientation(emitter, i, mMinOrientation, mMinOrientation, emitterWorldOrientation);
}

void CE::Internal::RandomScale(ParticleEmitterComponent& emitter, size_t i, glm::vec3 minScale, glm::vec3 maxScale, glm::vec3 emitterWorldScale)
{
	emitter.GetParticleSizes()[i] = Math::lerp(minScale, maxScale, Random::Value<float>()) * emitterWorldScale;
}

void CE::Internal::RandomOrientation(ParticleEmitterComponent& emitter, size_t i, glm::vec3 minOrientation, glm::vec3 maxOrientation, glm::quat emitterWorldOrientation)
{
	emitter.GetParticleOrientations()[i] = emitterWorldOrientation * glm::quat{ DEG2RAD(Random::Range(minOrientation, maxOrientation)) };
}

CE::MetaType CE::ParticleEmitterShapeAABB::Reflect()
{
	MetaType type = MetaType{ MetaType::T<ParticleEmitterShapeAABB>{}, "ParticleEmitterShapeAABB" };
	type.GetProperties().Add(Props::sIsScriptableTag);

	type.AddField(&ParticleEmitterShapeAABB::mMinPosition, "mMinPosition").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticleEmitterShapeAABB::mMaxPosition, "mMaxPosition").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticleEmitterShapeAABB::mMinScale, "mMinScale").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticleEmitterShapeAABB::mMaxScale, "mMaxScale").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticleEmitterShapeAABB::mMinOrientation, "mMinOrientation").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticleEmitterShapeAABB::mMaxOrientation, "mMaxOrientation").GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<ParticleEmitterShapeAABB>(type);
	return type;
}


CE::MetaType CE::ParticleEmitterShapeSphere::Reflect()
{
	MetaType type = MetaType{ MetaType::T<ParticleEmitterShapeSphere>{}, "ParticleEmitterShapeSphere" };
	type.GetProperties().Add(Props::sIsScriptableTag);

	type.AddField(&ParticleEmitterShapeSphere::mRadius, "mRadius").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticleEmitterShapeSphere::mMinScale, "mMinScale").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticleEmitterShapeSphere::mMaxScale, "mMaxScale").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticleEmitterShapeSphere::mMinOrientation, "mMinOrientation").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticleEmitterShapeSphere::mMaxOrientation, "mMaxOrientation").GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<ParticleEmitterShapeSphere>(type);
	return type;
}

