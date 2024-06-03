#include "Precomp.h"
#include "Systems/EnvironmentGeneratorSystem.h"

#include "Components/EnvironmentGeneratorComponent.h"
#include "Components/TransformComponent.h"
#include "World/World.h"
#include "World/Registry.h"
#include "Meta/MetaType.h"
#include "Utilities/Geometry2d.h"
#include "Utilities/Geometry3d.h"
#include "Utilities/Reflect/ReflectComponentType.h"

namespace Game::Internal
{
	struct PartOfGeneratedEnvironmentTag
	{
		friend CE::ReflectAccess;
		static CE::MetaType Reflect()
		{
			CE::MetaType metaType = CE::MetaType{CE::MetaType::T<PartOfGeneratedEnvironmentTag>{}, "PartOfGeneratedEnvironmentTag" };
			CE::ReflectComponentType<PartOfGeneratedEnvironmentTag>(metaType);
			return metaType;
		}
	};
}

void Game::EnvironmentGeneratorSystem::Update(CE::World& world, float)
{
	CE::Registry& reg = world.GetRegistry();
	const entt::entity generatorEntity = reg.View<EnvironmentGeneratorComponent>().front();

	if (generatorEntity == entt::null)
	{
		return;
	}


	const CE::TransformComponent* generatorTransform = reg.TryGet<CE::TransformComponent>(generatorEntity);

	if (generatorTransform == nullptr)
	{
		return;
	}

	const glm::vec2 generatorPosition = generatorTransform->GetWorldPosition2D();

	EnvironmentGeneratorComponent& generator = reg.Get<EnvironmentGeneratorComponent>(generatorEntity);

	// for each entity with PartOfGeneratedEnvironmentTag
	//		if entity is orphan AND further than mDestroyRadius
	//			destroy(entity);
	//		end
	// end
	const float destroyDist2 = generator.mDestroyRadius * generator.mDestroyRadius;
	for (const auto [entity, transform] : reg.View<CE::TransformComponent, Internal::PartOfGeneratedEnvironmentTag>().each())
	{
		if (glm::distance2(transform.GetWorldPosition2D(), generatorPosition) >= destroyDist2)
		{
			reg.Destroy(entity, true);
		}
	}

	for (const EnvironmentGeneratorComponent::Layer& layer : generator.mLayers)
	{
		if (layer.mObjects.empty())
		{
			continue;
		}

		const uint32 numOfCellsEachAxis = static_cast<uint32>(ceilf((generator.mGenerateRadius + generator.mGenerateRadius) / layer.mCellSize)) + 1u;

		const glm::vec2 generatorCellPos = generatorPosition - glm::vec2{ fmodf(generatorPosition.x, layer.mCellSize), fmodf(generatorPosition.y, layer.mCellSize) };
		const glm::vec2 previousGeneratorCellPos = generator.mLastGeneratedAroundPosition - glm::vec2{ fmodf(generator.mLastGeneratedAroundPosition.x, layer.mCellSize),
			fmodf(generator.mLastGeneratedAroundPosition.y, layer.mCellSize) };

		const glm::vec2 topLeft = generatorCellPos - glm::vec2{ static_cast<float>(numOfCellsEachAxis) * .5f * layer.mCellSize };

		const CE::TransformedDisk generationCircle{ generatorCellPos, generator.mGenerateRadius };
		const CE::TransformedDisk prevGenerationCircle{ previousGeneratorCellPos, generator.mGenerateRadius };

		for (uint32 cellX = 0; cellX < numOfCellsEachAxis; cellX++)
		{
			for (uint32 cellZ = 0; cellZ < numOfCellsEachAxis; cellZ++)
			{
				const glm::vec2 worldPosition = topLeft + layer.mCellSize *
					glm::vec2{ static_cast<float>(cellX), static_cast<float>(cellZ) };

				const CE::TransformedAABB cellAABB{ worldPosition, worldPosition + glm::vec2{ layer.mCellSize } };

				if (CE::AreOverlapping(cellAABB, prevGenerationCircle)
					|| !CE::AreOverlapping(cellAABB, generationCircle))
				{
					continue;
				}

				// TODO replace with perlin
				// const float noise = CE::Random::Value<float>();

				const uint32 objectIndex = CE::Random::Range(0u, static_cast<uint32>(layer.mObjects.size()));
				const EnvironmentGeneratorComponent::Layer::Object& objectToSpawn = layer.mObjects[objectIndex];
				const CE::AssetHandle<CE::Prefab> prefabToSpawn = objectToSpawn.mPrefab;

				if (prefabToSpawn == nullptr)
				{
					continue;
				}

				const entt::entity entity = reg.CreateFromPrefab(*prefabToSpawn);

				if (entity == entt::null)
				{
					continue;
				}

				reg.AddComponent<Internal::PartOfGeneratedEnvironmentTag>(entity);

				CE::TransformComponent* objectTransform = reg.TryGet<CE::TransformComponent>(entity);

				if (objectTransform == nullptr)
				{
					continue;
				}

				const glm::vec2 spawnPosition = cellAABB.GetCentre() +
					glm::vec2{
						CE::Random::Range(-layer.mMaxRandomOffset, layer.mMaxRandomOffset),
						CE::Random::Range(-layer.mMaxRandomOffset, layer.mMaxRandomOffset)
				};

				const uint32 orientation = CE::Random::Range(1u, layer.mNumberOfRandomRotations);
				const float angle = static_cast<float>(orientation) * (TWOPI / static_cast<float>(layer.mNumberOfRandomRotations));

				objectTransform->SetWorldPosition(spawnPosition);
				objectTransform->SetLocalOrientation(CE::sUp * angle);
			}
		}
	}

	generator.mLastGeneratedAroundPosition = generatorPosition;
	//	//
	//	// for each layer
	//	//		for each possible cell
	//	//			if cell.boundingBox.distance(mLastGeneratedAroundPosition) <= mGenerateRadius || cell.boundingBox.distance(currentPosition) >= mGenerateRadius
	//	//				continue;
	//	//			end
	//	//
	//	//			if (isOccupiedByObjectInEarlierLayer) continue;
	//	//	
	//	//			noise = getNoise(cell.position);
	//	//			prefab = getPrefab(noise);
	//	//	
	//	//			if (prefab == nullptr) continue;
	//	//	
	//	//			entity = spawnPrefab(prefab);
	//	//			applyRandomRotation(entity, cell.position);
	//	//			applyRandomOffset(entity, cell.position);
	//	//	
	//	//			add PartOfGeneratedEnvironmentTag to entity
	//	//		 end

	//	// This algorithm is deterministic, which means we do not
	//	// need to 'store' which objects we placed where

	//const auto view = world.GetRegistry().View<DifficultyScalingComponent>();

	//for (auto [entity, scalingComponent] : view.each())
	//{
	//	float time = (world.GetCurrentTimeScaled() - scalingComponent.mLoopsElapsed * scalingComponent.mScaleTime) / scalingComponent.mScaleTime;

	//	if (!scalingComponent.mIsRepeating)
	//	{
	//		time = glm::min(time, 1.0f);
	//	}
	//	else
	//	{
	//		if (time > 1.0f)
	//		{
	//			++scalingComponent.mLoopsElapsed;
	//			float minHealth = scalingComponent.mMinHealthMultiplier;
	//			float minDamage = scalingComponent.mMinDamageMultiplier;

	//			scalingComponent.mMinHealthMultiplier = scalingComponent.mMaxHealthMultiplier;
	//			scalingComponent.mMinDamageMultiplier = scalingComponent.mMaxDamageMultiplier;
	//			scalingComponent.mMaxHealthMultiplier += scalingComponent.mMaxHealthMultiplier - minHealth;
	//			scalingComponent.mMaxDamageMultiplier += scalingComponent.mMaxDamageMultiplier - minDamage;
	//		}

	//		time = glm::mod((world.GetCurrentTimeScaled() / scalingComponent.mScaleTime), 1.0f);
	//	}

	//	scalingComponent.mCurrentHealthMultiplier = scalingComponent.mScaleHPOverTime.GetValueAt(time) * (scalingComponent.mMaxHealthMultiplier - scalingComponent.mMinHealthMultiplier) + scalingComponent.mMinHealthMultiplier;
	//	scalingComponent.mCurrentDamageMultiplier = scalingComponent.mScaleDamageOverTime.GetValueAt(time) * (scalingComponent.mMaxDamageMultiplier - scalingComponent.mMinDamageMultiplier) + scalingComponent.mMinDamageMultiplier;
	//}
}

CE::MetaType Game::EnvironmentGeneratorSystem::Reflect()
{
	return CE::MetaType{ CE::MetaType::T<EnvironmentGeneratorSystem>{}, "EnvironmentGeneratorSystem", CE::MetaType::Base<System>{} };
}