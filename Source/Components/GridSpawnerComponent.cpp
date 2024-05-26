#include "Precomp.h"
#include "Components/GridSpawnerComponent.h"

#include "Components/TransformComponent.h"
#include "World/Registry.h"
#include "World/World.h"
#include "Assets/Prefabs/Prefab.h"
#include "Utilities/Random.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"

void CE::GridSpawnerComponent::OnConstruct(World&, entt::entity owner)
{
	mOwner = owner;
}

void CE::GridSpawnerComponent::OnBeginPlay(World&, entt::entity) const
{
	if (mShouldSpawnOnBeginPlay)
	{
		SpawnGrid();
	}
}

void CE::GridSpawnerComponent::ClearGrid() const
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

void CE::GridSpawnerComponent::SpawnGrid() const
{
	if (mTiles.empty())
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

	float total{};

	struct SpawnChance
	{
		AssetHandle<Prefab> mTile{};
		float mChance{};
	};
	std::vector<SpawnChance> chances{};

	for (uint32 i = 0; i < mTiles.size(); i++)
	{
		chances.emplace_back(SpawnChance{ mTiles[i], i < mSpawnChances.size() ? mSpawnChances[i] : 0.0f });
	}

	for (const SpawnChance& spawnChance : chances)
	{
		total += spawnChance.mChance;
	}

	std::sort(chances.begin(), chances.end(),
		[](const SpawnChance& Left, const SpawnChance& Right)
		{
			return Left.mChance < Right.mChance;
		});

	// Normalize and cumulate
	float cumulative{};
	for (SpawnChance& spawnChance : chances)
	{
		spawnChance.mChance /= total;

		spawnChance.mChance += cumulative;
		cumulative = spawnChance.mChance;
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

			const double randomNum = Random::Range(0.0f, 1.0f);

			int32 bestIndex = 0;
			int32 numChances = static_cast<int32>(chances.size());

			for (int32 i = 1; i < numChances; i++)
			{
				const float Value = chances[i].mChance > randomNum ? chances[i].mChance : INFINITY;
				const float BestValue = chances[bestIndex].mChance > randomNum ? chances[bestIndex].mChance : INFINITY;

				if (Value < BestValue)
				{
					bestIndex = i;
				}
			}

			AssetHandle<Prefab> tile = chances[bestIndex].mTile;

			if (tile == nullptr)
			{
				continue;
			}

			const entt::entity entity = reg.CreateFromPrefab(*tile);

			TransformComponent* child = reg.TryGet<TransformComponent>(entity);

			if (child == nullptr)
			{
				continue;
			}

			child->SetParent(transform);
			child->SetLocalPosition(position);

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
	type.AddField(&GridSpawnerComponent::mTiles, "mTiles").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&GridSpawnerComponent::mSpawnChances, "mChances").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&GridSpawnerComponent::mIsCentered, "mIsCentered").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&GridSpawnerComponent::mShouldSpawnOnBeginPlay, "mShouldSpawnOnBeginPlay").GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc(&GridSpawnerComponent::SpawnGrid, "Spawn Grid").GetProperties().Add(Props::sCallFromEditorTag).Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);
	type.AddFunc(&GridSpawnerComponent::ClearGrid, "Clear Grid").GetProperties().Add(Props::sCallFromEditorTag).Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	BindEvent(type, sConstructEvent, &GridSpawnerComponent::OnConstruct);
	BindEvent(type, sBeginPlayEvent, &GridSpawnerComponent::OnBeginPlay);

	ReflectComponentType<GridSpawnerComponent>(type);
	return type;
}
