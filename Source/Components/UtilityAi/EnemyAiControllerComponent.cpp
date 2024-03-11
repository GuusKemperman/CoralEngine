#include "Precomp.h"
#include "Components/UtililtyAi/EnemyAiControllerComponent.h"

#include "Utilities/Reflect/ReflectComponentType.h"
#include "World/Registry.h"
#include "Utilities/Events.h"

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

		const MetaFunc* componentAiEvaluate = TryGetEvent(*type, sAIEvaluateEvent);
		const MetaFunc* componentAiTick = TryGetEvent(*type, sAITickEvent);

		// focus on cleaning this
		if (!storage.contains(enemyID) || !componentAiEvaluate || !componentAiTick)
		{
			continue;
		}

		MetaAny component{*type, storage.value(enemyID), false};

		const bool isStatic = componentAiEvaluate->GetProperties().Has(Props::sIsEventStaticTag);

		FuncResult fr;
		if (isStatic)
		{
			fr = componentAiEvaluate->InvokeCheckedUnpacked(world, enemyID);
		}
		else
		{
			fr = componentAiEvaluate->InvokeCheckedUnpacked(component, world, enemyID);
		}

		if (fr.HasError())
		{
			//log here (I forgor)
			continue;
		}

		const float* score = fr.GetReturnValue().As<float>();
		if (*score > bestScore)
		{
			bestScore = *score;
			bestType = type;
			bestStorage = &storage;
		}
	}

	if (bestType != nullptr)
	{
		MetaAny component(*bestType, bestStorage->value(enemyID), false);
		const MetaFunc* componentAiTick = TryGetEvent(*bestType, sAITickEvent);

		const bool isStatic = componentAiTick->GetProperties().Has(Props::sIsEventStaticTag);
		if (isStatic)
		{
			FuncResult fr = componentAiTick->InvokeCheckedUnpacked(world, enemyID, dt);
		}
		else
		{
			FuncResult fr = componentAiTick->InvokeCheckedUnpacked(component, world, enemyID, dt);
		}
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
