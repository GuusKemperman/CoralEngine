#pragma once
#include "Assets/Core/AssetHandle.h"
#include "BasicDataTypes/Bezier.h"
#include "BasicDataTypes/Colors/ColorGradient.h"
#include "BasicDataTypes/Colors/LinearColor.h"
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

				float mFrequency = 1.0f;

#ifdef EDITOR
				void DisplayWidget(const std::string& name);
#endif // EDITOR

				bool operator==(const Object& other) const;
				bool operator!=(const Object& other) const;

			private:
				friend CE::ReflectAccess;
				static CE::MetaType Reflect();
			};
			std::vector<Object> mObjects{};

			// Objects can only spawn when
			// the noise at that position is
			// within this range.
			float mMinNoiseValueToSpawn = 0.0f;
			float mMaxNoiseValueToSpawn = 1.0f;

			// The chance of spawning any of the objects,
			// if the noise at that position is
			// in an acceptable range.
			float mObjectSpawnChance = 1.0f;

			// We divide the layer into smaller
			// cells, or 'tiles'. For each tile
			// we determine which object to spawn.
			float mCellSize = 1.0f;

			// Each spawned object can receive
			// a random offset to its position.
			float mMaxRandomOffset{};
			float mMinRandomHeightOffset{};
			float mMaxRandomHeightOffset{};

			std::vector<CE::LinearColor> mColors{};

			// By default, we spawn objects
			// if their cell overlaps with our
			// generate-range. This becomes a
			// problem if the cell size is very
			// large, as objects are then spawned
			// even when its very far away from
			// the player.
			std::optional<float> mObjectsRadius{};

			uint32 mNumberOfRandomRotations = 4;

			glm::vec2 mMinRandomOrientation{};
			glm::vec2 mMaxRandomOrientation{};

			// Can be used to, for example, scale down trees near the edges
			// of forests
			CE::Bezier mScaleAtNoiseValue{ { glm::vec2{ 0.0f, 1.0f }, glm::vec2{ 1.0f, 1.0f }, glm::vec2{ -1.0f } } };

			// Number that determines at what distance to view the noisemap.
			float mNoiseScale = 0.02f;

			// The number of levels of detail you want you perlin noise to have.
			uint32 mNoiseNumOfOctaves = 1;

			// Number that determines how much each octave contributes to the overall shape(adjusts amplitude).
			float mNoisePersistence = .5f;

			struct NoiseInfluence
			{
				uint32 mIndexOfOtherLayer{};
				float mWeight = 1.0f;

#ifdef EDITOR
				void DisplayWidget(const std::string& name);
#endif // EDITOR

				bool operator==(const NoiseInfluence& other) const;
				bool operator!=(const NoiseInfluence& other) const;

			private:
				friend CE::ReflectAccess;
				static CE::MetaType Reflect();
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

			bool mIsDebugDrawingEnabled = true;

#ifdef EDITOR
			void DisplayWidget(const std::string& name);
#endif // EDITOR

			bool operator==(const Layer& other) const;
			bool operator!=(const Layer& other) const;

		private:
			friend CE::ReflectAccess;
			static CE::MetaType Reflect();
		};
		std::vector<Layer> mLayers{};

		float mGenerateRadius = 50.0f;
		float mDestroyRadius = 80.0f;

		float mDistToMoveBeforeRegeneration = 10.0f;

		uint32 mSeed = CE::Random::Value<uint32>();

		glm::vec2 mLastGeneratedAtPosition{ std::numeric_limits<float>::infinity() };

		float mDebugDrawNoiseHeight = 5.0f;
		float mDebugDrawDistanceBetweenLayers = 5.0f;

		bool mShouldGenerateInEditor{};
		bool mWasClearingRequested{};

	private:
		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(EnvironmentGeneratorComponent);
	};
}

CEREAL_CLASS_VERSION(Game::EnvironmentGeneratorComponent::Layer, 6);
CEREAL_CLASS_VERSION(Game::EnvironmentGeneratorComponent::Layer::Object, 2);
CEREAL_CLASS_VERSION(Game::EnvironmentGeneratorComponent::Layer::NoiseInfluence, 0);

namespace cereal
{
	template<class Archive>
	void serialize(Archive& ar, Game::EnvironmentGeneratorComponent::Layer& value, uint32 version)
	{
		ar(value.mObjects, 
			value.mCellSize, 
			value.mMaxRandomOffset, 
			value.mNumberOfRandomRotations, 
			value.mNoiseScale, 
			value.mNoiseNumOfOctaves,
			value.mNoisePersistence,
			value.mInfluences,
			value.mWeight);

		if (version >= 1)
		{
			ar(value.mIsDebugDrawingEnabled);
		}

		if (version >= 2)
		{
			ar(value.mScaleAtNoiseValue, value.mMinNoiseValueToSpawn, value.mMaxNoiseValueToSpawn, value.mObjectSpawnChance);
		}

		if (version >= 3)
		{
			ar(value.mObjectsRadius);
		}

		if (version >= 4)
		{
			ar(value.mMinRandomHeightOffset, value.mMaxRandomHeightOffset, value.mMinRandomOrientation, value.mMaxRandomOrientation);
		}

		if (version == 5)
		{
			CE::ColorGradient dummy{};
			ar(dummy);
		}

		if (version >= 6)
		{
			ar(value.mColors);
		}
	}

	template<class Archive>
	void serialize(Archive& ar, Game::EnvironmentGeneratorComponent::Layer::Object& value, uint32 version)
	{
		ar(value.mFrequency, value.mPrefab);

		if (version == 0)
		{
			CE::Bezier dummy{};
			ar(dummy);
		}

		if (version == 1)
		{
			CE::Bezier dummy{};
			ar(dummy);
			
		}
	}

	template<class Archive>
	void serialize(Archive& ar, Game::EnvironmentGeneratorComponent::Layer::NoiseInfluence& value, uint32)
	{
		ar(value.mWeight, value.mIndexOfOtherLayer);
	}
}

#ifdef EDITOR
IMGUI_AUTO_DEFINE_INLINE(template<>, Game::EnvironmentGeneratorComponent::Layer, var.DisplayWidget(name);)
IMGUI_AUTO_DEFINE_INLINE(template<>, Game::EnvironmentGeneratorComponent::Layer::Object, var.DisplayWidget(name);)
IMGUI_AUTO_DEFINE_INLINE(template<>, Game::EnvironmentGeneratorComponent::Layer::NoiseInfluence, var.DisplayWidget(name);)
#endif // EDITOR