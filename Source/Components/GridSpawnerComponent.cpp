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

	CE::DefaultRandomEngine cellGenerator{ Random::CreateSeed(transform->GetWorldPosition2D()) };

	const std::function<uint32(uint32, uint32)> randomUint = mUseWorldPositionAsSeed ?
		std::function
		{
			[&cellGenerator](uint32 min, uint32 max)
			{
				std::uniform_int_distribution distribution{ min, std::max(min, max == min ? max : max - 1) };
				return distribution(cellGenerator);
			}
		}
		:
		std::function
		{
			[](uint32 min, uint32 max)
			{
				return Random::Range(min, max);
			}
		};

	const auto randomFloat = mUseWorldPositionAsSeed ?
		std::function
		{
			[&cellGenerator](float min, float max)
			{
				std::uniform_real_distribution distribution{ min, std::max(min, max) };
				return distribution(cellGenerator);
			}
		}
		:
		std::function
		{
			[](float min, float max)
			{
				return Random::Range(min, max);
			}
		};

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

			const AssetHandle<Prefab>* tile = mDistribution.GetNext(randomFloat(0.0f, 1.0f));

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

			const glm::vec2 offset = { randomFloat(0.0f, mMaxRandomOffset), randomFloat(0.0f, mMaxRandomOffset) };
			child->SetLocalPosition(To3DRightForward(position + offset, randomFloat(mMinRandomHeightOffset, mMaxRandomHeightOffset)));

			child->SetLocalScale(randomFloat(mMinRandomScale, mMaxRandomScale));

			const uint32 orientation = randomUint(1u, mNumOfPossibleRotations);
			const float angle = static_cast<float>(orientation) * angleStep;
			glm::quat quatOrientation{ glm::vec3{ sUp * angle } };

			quatOrientation *= glm::quat{ sRight * glm::radians(randomFloat(mMinRandomOrientation.x, mMaxRandomOrientation.x)) };
			quatOrientation *= glm::quat{ sForward * glm::radians(randomFloat(mMinRandomOrientation.y, mMaxRandomOrientation.y)) };

			child->SetLocalOrientation(quatOrientation);
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
	type.AddField(&GridSpawnerComponent::mMinRandomHeightOffset, "mMinRandomHeightOffset").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&GridSpawnerComponent::mMaxRandomHeightOffset, "mMaxRandomHeightOffset").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&GridSpawnerComponent::mMinRandomScale, "mMinRandomScale").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&GridSpawnerComponent::mMaxRandomScale, "mMaxRandomScale").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&GridSpawnerComponent::mMinRandomOrientation, "mMinRandomOrientation").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&GridSpawnerComponent::mMaxRandomOrientation, "mMaxRandomOrientation").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&GridSpawnerComponent::mDistribution, "mDistribution").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&GridSpawnerComponent::mIsCentered, "mIsCentered").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&GridSpawnerComponent::mShouldSpawnOnBeginPlay, "mShouldSpawnOnBeginPlay").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&GridSpawnerComponent::mUseWorldPositionAsSeed, "mUseWorldPositionAsSeed").GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc(&GridSpawnerComponent::SpawnGrid, "Spawn Grid").GetProperties().Add(Props::sCallFromEditorTag).Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);
	type.AddFunc(&GridSpawnerComponent::ClearGrid, "Clear Grid").GetProperties().Add(Props::sCallFromEditorTag).Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	BindEvent(type, sConstructEvent, &GridSpawnerComponent::OnConstruct);
	BindEvent(type, sBeginPlayEvent, &GridSpawnerComponent::OnBeginPlay);

	ReflectComponentType<GridSpawnerComponent>(type);
	return type;
}
