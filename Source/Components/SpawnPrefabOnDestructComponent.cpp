#include "precomp.h"
#include "Components/SpawnPrefabOnDestructComponent.h"

#include "Assets/Prefabs/Prefab.h"
#include "Components/TransformComponent.h"
#include "World/World.h"
#include "World/Registry.h"
#include "Meta/MetaType.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void CE::SpawnPrefabOnDestructComponent::OnDestruct(World& world, entt::entity entity)
{
	const TransformComponent* transform = world.GetRegistry().TryGet<TransformComponent>(entity);

	if (transform == nullptr)
	{
		LOG(LogWorld, Warning, "Tried to construct a Prefab but no transform was found");
		return;
	}

	if (mPrefab == nullptr)
	{
		LOG(LogWorld, Warning, "SpawnPrefabOnDestructComponent does not have a prefab to spawn");
		return;
	}

	entt::entity newEntity = world.GetRegistry().CreateFromPrefab(*mPrefab);

	TransformComponent* newTransform = world.GetRegistry().TryGet<TransformComponent>(newEntity);

	if (newTransform != nullptr)
	{
		newTransform->SetWorldMatrix(transform->GetWorldMatrix());
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
