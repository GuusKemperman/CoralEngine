#include "Precomp.h"
#include "Components/UtililtyAi/EnemyAiControllerComponent.h"

#include "Utilities/Reflect/ReflectComponentType.h"
#include "World/Registry.h"
#include "Utilities/Events.h"

Engine::EnemyAiControllerComponent::EnemyAiControllerComponent()
{
}

void Engine::EnemyAiControllerComponent::UpdateState(World& world, entt::entity enemyID, float dt)
{
	Registry& reg = world.GetRegistry();

	float bestScore = std::numeric_limits<float>::lowest();
	const MetaType* bestType = nullptr;
	entt::basic_sparse_set<>* bestStorage = nullptr;


	for (auto&& [typeHash, storage] : reg.Storage())
	{
		const MetaType* type = MetaManager::Get().TryGetType(typeHash);

		if (type == nullptr)
		{
			continue;
		}

		if (storage.contains(enemyID) && TryGetEvent(*type, sAITickEvent))
		{
			MetaAny component(type, storage.value(enemyID));

			const MetaFunc& componentAiEvaluate = *TryGetEvent(*type, sAIEvaluateEvent);
			FuncResult fr = componentAiEvaluate(component, world, enemyID, dt);

			float score = fr.GetReturnValue().IsExactly<float>();

			if (score > bestScore)
			{
				bestScore = score;
				bestType = type;
				bestStorage = &storage;
			}
		}
	}

	if (bestType != nullptr)
	{
		MetaAny component(*bestType, bestStorage->value(enemyID));

		const MetaFunc& componentAiTick = *TryGetEvent(*bestType, sAITickEvent);
		FuncResult fr = componentAiTick(component, world, enemyID, dt);
	}
}

Engine::MetaType Engine::EnemyAiControllerComponent::Reflect()
{
	auto type = MetaType{MetaType::T<EnemyAiControllerComponent>{}, "EnemyAIControllerComponent"};
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	ReflectComponentType<EnemyAiControllerComponent>(type);
	return type;
}
