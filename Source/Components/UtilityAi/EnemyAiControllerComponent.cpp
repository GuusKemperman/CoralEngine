#include "Precomp.h"
#include "Components/UtililtyAi/EnemyAiControllerComponent.h"

#include "Utilities/Reflect/ReflectComponentType.h"
#include "World/Registry.h"

Engine::EnemyAiControllerComponent::EnemyAiControllerComponent()
{
}

void Engine::EnemyAiControllerComponent::UpdateState(World& world, entt::entity enemyID)
{
	Registry& reg = world.GetRegistry();

	for (auto&& [typeHash, storage] : reg.Storage())
	{
		const MetaType* const type = MetaManager::Get().TryGetType(typeHash);

		if (type == nullptr)
		{
			continue;
		}

		if (storage.contains(enemyID))
		{
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
