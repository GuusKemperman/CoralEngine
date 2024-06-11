#include "Precomp.h"
#include "Components/Particles/ParticleEmitterComponent.h"

#include "Components/Particles/ParticleProperty.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"

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
	ReflectComponentType<ParticleEmitterComponent>(type);
	return type;
}
