#include "Precomp.h"
#include "Components/Particles/ParticleUtilities.h"

#include "Components/NameComponent.h"

void CE::Internal::OnParticleComponentDestruct(World& world, entt::entity oldEntity)
{
	Registry& reg = world.GetRegistry();
	ParticleEmitterComponent* const emitter = reg.TryGet<ParticleEmitterComponent>(oldEntity);

	if (emitter == nullptr
		|| !emitter->mKeepParticlesAliveWhenEmitterIsDestroyed
		|| emitter->mOnlyStayAliveUntilExistingParticlesAreGone
		|| emitter->GetNumOfParticles() == 0)
	{
		return;
	}

	const entt::entity newEntity = reg.Create();

	if (NameComponent* name = reg.TryGet<NameComponent>(oldEntity))
	{
		reg.AddComponent<NameComponent>(newEntity, std::move(*name));
	}

	if (TransformComponent* oldTransform = reg.TryGet<TransformComponent>(oldEntity))
	{
		reg.AddComponent<TransformComponent>(newEntity, std::move(*oldTransform));
	}

	static std::vector<std::reference_wrapper<const MetaFunc>> particleTypes =
		[]
		{
			std::vector<std::reference_wrapper<const MetaFunc>> types{};
			for (const MetaType& type : MetaManager::Get().EachType())
			{
				if (const MetaFunc* func = type.TryGetFunc(sTransferOwnershipName); func != nullptr)
				{
					types.emplace_back(*func);
				}
			}
			return types;
		}();

	for (const MetaFunc& func : particleTypes)
	{
		func.InvokeUncheckedUnpacked(reg, oldEntity, newEntity);
	}

	// Prevent the destruct events called for
	// the other components from calling this function again
	emitter->mKeepParticlesAliveWhenEmitterIsDestroyed = false;

	ParticleEmitterComponent& newEmitter = reg.Get<ParticleEmitterComponent>(newEntity);
	newEmitter.mOnlyStayAliveUntilExistingParticlesAreGone = true;
}
