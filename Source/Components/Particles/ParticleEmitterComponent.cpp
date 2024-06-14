#include "Precomp.h"
#include "Components/Particles/ParticleEmitterComponent.h"

#include "Components/Particles/ParticleUtilities.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"

glm::vec3 CE::ParticleEmitterComponent::GetParticlePositionFast(uint32 particle) const
{
	return mParticlePositions[particle];
}

void CE::ParticleEmitterComponent::SetParticlePositionFast(uint32 particle, glm::vec3 position)
{
	mParticlePositions[particle] = position;
}

glm::vec3 CE::ParticleEmitterComponent::GetParticlePositionWorld(uint32 particle) const
{
	if (!mAreTransformsRelativeToEmitter)
	{
		return GetParticlePositionFast(particle);
	}

	return mEmitterWorldMatrix * glm::vec4{ GetParticlePositionFast(particle), 1.0f };
}

void CE::ParticleEmitterComponent::SetParticlePositionWorld(uint32 particle, glm::vec3 position)
{
	if (!mAreTransformsRelativeToEmitter)
	{
		SetParticlePositionFast(particle, position);
	}

	SetParticlePositionFast(particle, mInverseEmitterWorldMatrix * glm::vec4{ GetParticlePositionFast(particle), 1.0f });
}

glm::quat CE::ParticleEmitterComponent::GetParticleOrientationFast(uint32 particle) const
{
	return mParticleOrientations[particle];
}

void CE::ParticleEmitterComponent::SetParticleOrientationFast(uint32 particle, glm::quat orientation)
{
	mParticleOrientations[particle] = orientation;
}

glm::quat CE::ParticleEmitterComponent::GetParticleOrientationWorld(uint32 particle) const
{
	if (!mAreTransformsRelativeToEmitter)
	{
		return GetParticleOrientationFast(particle);
	}

	return mEmitterOrientation * GetParticleOrientationFast(particle);
}

void CE::ParticleEmitterComponent::SetParticleOrientationWorld(uint32 particle, glm::quat orientation)
{
	if (!mAreTransformsRelativeToEmitter)
	{
		SetParticleOrientationFast(particle, orientation);
	}

	SetParticleOrientationFast(particle, mInverseEmitterOrientation * GetParticleOrientationFast(particle));
}

void CE::ParticleEmitterComponent::PlayFromStart()
{
	mCurrentTime = 0.0f;
	mNumOfParticlesToSpawnNextFrame = 0.0f;

	if (!mKeepExistingParticlesAliveWhenRestartingLoop)
	{
		for (float& lifeTimeAsPercentage : mParticleTimeAsPercentage)
		{
			lifeTimeAsPercentage = 1.00001f;
		}
	}
}

CE::MetaType CE::ParticleEmitterComponent::Reflect()
{
	MetaType type = MetaType{ MetaType::T<ParticleEmitterComponent>{}, "ParticleEmitterComponent" };
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	type.AddField(&ParticleEmitterComponent::mNumOfParticlesToSpawn, "mNumOfParticlesToSpawn").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticleEmitterComponent::mParticleSpawnRateOverTime, "mParticleSpawnRateOverTime");
	type.AddField(&ParticleEmitterComponent::mMinLifeTime, "mMinLifeTime").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticleEmitterComponent::mMaxLifeTime, "mMaxLifeTime").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticleEmitterComponent::mScale, "mScale").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticleEmitterComponent::mLoop, "mLoop").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticleEmitterComponent::mKeepExistingParticlesAliveWhenRestartingLoop, "mKeepExistingParticlesAliveWhenRestartingLoop").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticleEmitterComponent::mKeepParticlesAliveWhenEmitterIsDestroyed, "mKeepParticlesAliveWhenEmitterIsDestroyed").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticleEmitterComponent::mAreTransformsRelativeToEmitter, "mMoveParticlesWithEmitter").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticleEmitterComponent::mDestroyOnFinish, "mDestroyOnFinish").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticleEmitterComponent::mIsPaused, "mIsPaused").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticleEmitterComponent::mDuration, "mDuration").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticleEmitterComponent::mCurrentTime, "mCurrentTime").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&ParticleEmitterComponent::IsPaused, "IsPaused", "").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&ParticleEmitterComponent::IsPlaying, "IsPlaying", "").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&ParticleEmitterComponent::GetNumOfParticles, "GetNumOfParticles", "").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&ParticleEmitterComponent::DidParticleJustSpawn, "DidParticleJustSpawn", "", "particle").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&ParticleEmitterComponent::IsParticleAlive, "IsParticleAlive", "", "particle").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&ParticleEmitterComponent::PlayFromStart, "PlayFromStart", "").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sCallFromEditorTag);

	ReflectParticleComponentType<ParticleEmitterComponent>(type);
	return type;
}
