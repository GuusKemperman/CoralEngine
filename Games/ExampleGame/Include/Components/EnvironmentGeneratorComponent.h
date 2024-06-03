#pragma once
#include "Assets/Core/AssetHandle.h"
#include "BasicDataTypes/Bezier.h"
#include "Utilities/WeightedRandomDistribution.h"

namespace CE
{
	class Prefab;
}

namespace Game
{
	class EnvironmentGeneratorComponent
	{
	public:
		struct Layer
		{
			struct Object
			{
				CE::AssetHandle<CE::Prefab> mPrefab{};

				float mBaseFrequency{};

				// Can be used to, for example, make 'larger' rocks only spawn
				// if the noise value is really high. This leads to larger rocks
				// in the centre surrounded by a bunch of smaller rocks.
				//
				// The actual frequency used for determining which object to spawn is:
				// spawnChance = mBaseFrequency * mSpawnFrequenciesAtNoiseValue.At(noiseValue).
				CE::Bezier mSpawnFrequenciesAtNoiseValue{};

				// Can be used to, for example, scale down trees near the edges
				// of forests
				CE::Bezier mScaleAtNoiseValue{};
			};
			std::vector<Object> mObjects{};

			// We divide the layer into smaller
			// cells, or 'tiles'. For each tile
			// we determine which object to spawn.
			float mCellSize{};

			// Each spawned object can receive
			// a random offset to its position.
			float mMaxRandomOffset{};

			uint32 mNumberOfRandomRotations = 4;

			// Number that determines at what distance to view the noisemap.
			float mNoiseScale = 1.0f;

			// The number of levels of detail you want you perlin noise to have.
			uint32 mNoiseNumOfOctaves{};

			// Number that determines how much each octave contributes to the overall shape(adjusts amplitude).
			float mNoisePersistence = .5f;

			struct NoiseInfluence
			{
				uint32 mIndexOfOtherLayer{};
				float mWeight = 1.0f;
			};

			// Can be used to have relations between
			// layers, for example to have different
			// tiles in foresty areas.
			std::vector<NoiseInfluence> mInfluences{};

			// Sometimes you only want the noise
			// to be based on the noise from other
			// layers. You'll have a lot of entries
			// in mInfluences, but all of those
			// would be offset by your own. This
			// value can be set to 0.0f to nullify
			// the effects of your own noise.
			float mWeight = 1.0f;
		};
		std::vector<Layer> mLayers{};

		float mGenerateRadius{};
		float mDestroyRadius{};

		// for each entity with PartOfGeneratedEnvironmentTag
		//		if entity is orphan AND further than mDestroyRadius
		//			destroy(entity);
		//		end
		// end
		//
		// for each layer
		//		for each possible cell
		//			if cell.boundingBox.distance(mLastGeneratedAroundPosition) <= mGenerateRadius || cell.boundingBox.distance(currentPosition) >= mGenerateRadius
		//				continue;
		//			end
		//
		//			if (isOccupiedByObjectInEarlierLayer) continue;
		//	
		//			noise = getNoise(cell.position);
		//			prefab = getPrefab(noise);
		//	
		//			if (prefab == nullptr) continue;
		//	
		//			entity = spawnPrefab(prefab);
		//			applyRandomRotation(entity, cell.position);
		//			applyRandomOffset(entity, cell.position);
		//	
		//			for each child (recursively)
		//				add PartOfGeneratedEnvironmentTag to entity
		//				markGroundUnderneathAsOccupied(child);
		//			end
		//		 end

		// This algorithm is deterministic, which means we do not
		// need to 'store' which objects we placed where

		// Is used to determine which 
		glm::vec2 mLastGeneratedAroundPosition{};
	};
}
