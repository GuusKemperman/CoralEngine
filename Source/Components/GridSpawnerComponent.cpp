#include "Precomp.h"
#include "Components/GridSpawnerComponent.h"

#include "Components/TransformComponent.h"
#include "World/Registry.h"
#include "World/World.h"
#include "Assets/Prefabs/Prefab.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void CE::GridSpawnerComponent::OnConstruct(World&, entt::entity owner)
{
	mOwner = owner;
}

void CE::GridSpawnerComponent::OnBeginPlay(World&, entt::entity)
{
	if (mShouldSpawnOnBeginPlay)
	{
		SpawnGrid();
	}
}

void CE::GridSpawnerComponent::ClearGrid()
{
	World* const world = World::TryGetWorldAtTopOfStack();

	if (world == nullptr)
	{
		return;
	}

	Registry& reg = world->GetRegistry();

	TransformComponent* transform = reg.TryGet<TransformComponent>(mOwner);

	if (transform == nullptr)
	{
		return;
	}

	for (const TransformComponent& child : transform->GetChildren())
	{
		reg.Destroy(child.GetOwner(), true);
	}
}

void CE::GridSpawnerComponent::SpawnGrid()
{
	if (mDistribution.mWeights.empty())
	{
		return;
	}

	World* const world = World::TryGetWorldAtTopOfStack();

	if (world == nullptr)
	{
		return;
	}

	ClearGrid();

	Registry& reg = world->GetRegistry();
	TransformComponent* transform = reg.TryGet<TransformComponent>(mOwner);

	if (transform == nullptr)
	{
		return;
	}
	const float angleStep = TWOPI / static_cast<float>(mNumOfPossibleRotations);

	for (uint32 x = 0; x < mWidth; x++)
	{
		for (uint32 y = 0; y < mHeight; y++)
		{
			glm::vec2 position =
			{
				static_cast<float>(x) * mSpacing.x,
				static_cast<float>(y) * mSpacing.y
			};

			if (mIsCentered)
			{
				position.x -= static_cast<float>(mWidth - 1) * mSpacing.x * .5f;
				position.y -= static_cast<float>(mHeight - 1) * mSpacing.y * .5f;
			}

			const AssetHandle<Prefab>* tile = mDistribution.GetNext();

			if (tile == nullptr
				|| *tile == nullptr)
			{
				continue;
			}

			const entt::entity entity = reg.CreateFromPrefab(**tile);

			TransformComponent* child = reg.TryGet<TransformComponent>(entity);

			if (child == nullptr)
			{
				continue;
			}

			child->SetParent(transform);

			const glm::vec2 offset = { Random::Range(0.0f, mMaxRandomOffset), Random::Range(0.0f, mMaxRandomOffset) };
			child->SetLocalPosition(position + offset);

			const uint32 orientation = Random::Range(1u, mNumOfPossibleRotations);
			const float angle = static_cast<float>(orientation) * angleStep;
			child->SetLocalOrientation(sUp * angle);
		}
	}
}

CE::MetaType CE::GridSpawnerComponent::Reflect()
{
	auto type = MetaType{ MetaType::T<GridSpawnerComponent>{}, "GridSpawnerComponent" };
	MetaProps& props = type.GetProperties();
	props.Set(Props::sOldNames, "GridSpawner");
	props.Add(Props::sIsScriptableTag);

	type.AddField(&GridSpawnerComponent::mSpacing, "mSpacing").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&GridSpawnerComponent::mHeight, "mHeight").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&GridSpawnerComponent::mWidth, "mWidth").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&GridSpawnerComponent::mNumOfPossibleRotations, "mNumOfPossibleRotations").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&GridSpawnerComponent::mMaxRandomOffset, "mMaxRandomOffset").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&GridSpawnerComponent::mDistribution, "mDistribution").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&GridSpawnerComponent::mIsCentered, "mIsCentered").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&GridSpawnerComponent::mShouldSpawnOnBeginPlay, "mShouldSpawnOnBeginPlay").GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc(&GridSpawnerComponent::SpawnGrid, "Spawn Grid").GetProperties().Add(Props::sCallFromEditorTag).Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);
	type.AddFunc(&GridSpawnerComponent::ClearGrid, "Clear Grid").GetProperties().Add(Props::sCallFromEditorTag).Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	BindEvent(type, sConstructEvent, &GridSpawnerComponent::OnConstruct);
	BindEvent(type, sBeginPlayEvent, &GridSpawnerComponent::OnBeginPlay);

	ReflectComponentType<GridSpawnerComponent>(type);
	return type;
}
