#include "Precomp.h"
#include "Components/Particles/ParticleUtilities.h"

void CE::Internal::OnParticleComponentDestruct(World& world, entt::entity entity)
{
	Registry& reg = world.GetRegistry();
	ParticleEmitterComponent* const emitter = reg.TryGet<ParticleEmitterComponent>(entity);

	if (emitter == nullptr
		|| !emitter->mKeepParticlesAliveWhenEmitterIsDestroyed
		|| emitter->mOnlyStayAliveUntilExistingParticlesAreGone
		|| emitter->GetNumOfParticles() == 0)
	{
		return;
	}

	const entt::entity newEntity = reg.Create();
	reg.AddComponent<TransformComponent>(newEntity).SetWorldMatrix(emitter->mEmitterWorldMatrix);

	std::vector<std::reference_wrapper<const MetaFunc>> particleTypes =
		[]
		{
			std::vector<std::reference_wrapper<const MetaFunc>> types{};
			for (const MetaType& type : MetaManager::Get().EachType())
			{
				if (const MetaFunc* func = type.TryGetFunc(sTransferOwnershipName); func != nullptr)
				{
					std::cout << type.GetName() << std::endl;
					types.emplace_back(*func);
				}
			}
			return types;
		}();

	for (const MetaFunc& func : particleTypes)
	{
		func.InvokeUncheckedUnpacked(reg, entity, newEntity);
	}

	// Prevent the destruct events called for
	// the other components from calling this function again
	emitter->mKeepParticlesAliveWhenEmitterIsDestroyed = false;

	ParticleEmitterComponent& newEmitter = reg.Get<ParticleEmitterComponent>(newEntity);
	newEmitter.mOnlyStayAliveUntilExistingParticlesAreGone = true;
}
