#include "precomp.h"
#include "Components/SpawnPrefabOnDestructComponent.h"

#include "Assets/Prefabs/Prefab.h"
#include "Components/TransformComponent.h"
#include "World/World.h"
#include "World/Registry.h"
#include "Meta/MetaType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void CE::SpawnPrefabOnDestructComponent::OnDestruct(World& world, entt::entity entity)
{
	if (mPrefab.empty())
	{
		LOG(LogWorld, Warning, "SpawnPrefabOnDestructComponent does not have any prefabs to spawn");
		return;
	}

	const TransformComponent* transform = world.GetRegistry().TryGet<TransformComponent>(entity);
	
	for (auto& prefab : mPrefab)
	{
		entt::entity newEntity = world.GetRegistry().CreateFromPrefab(*prefab);
		
		TransformComponent* newTransform = world.GetRegistry().TryGet<TransformComponent>(newEntity);
		
		if (transform != nullptr
			&& newTransform != nullptr)
		{
			newTransform->SetWorldMatrix(transform->GetWorldMatrix());
		}
	}
}

CE::MetaType CE::SpawnPrefabOnDestructComponent::Reflect()
{
	auto type = MetaType{MetaType::T<SpawnPrefabOnDestructComponent>{}, "SpawnPrefabOnDestructComponent"};
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);

	BindEvent(type, CE::sDestructEvent, &SpawnPrefabOnDestructComponent::OnDestruct);

	type.AddField(&SpawnPrefabOnDestructComponent::mPrefab, "mPrefab").GetProperties().Add(CE::Props::sIsScriptableTag);

	ReflectComponentType<SpawnPrefabOnDestructComponent>(type);
	return type;
}
