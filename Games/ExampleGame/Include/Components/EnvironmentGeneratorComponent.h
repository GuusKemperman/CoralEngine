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

			// We divide the layer into smaller
			// cells, or 'tiles'. For each tile
			// we determine which object to spawn.
			float mCellSize = 1.0f;

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

			bool mCanSpawnInOccupiedSpace = false;

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

		float mGenerateRadius = 200.0f;

		float mDistToMoveBeforeRegeneration = 50.0f;

		glm::vec2 mLastGeneratedAtPosition{ std::numeric_limits<float>::infinity() };

	private:
		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(EnvironmentGeneratorComponent);
	};
}

CEREAL_CLASS_VERSION(Game::EnvironmentGeneratorComponent::Layer, 0);
CEREAL_CLASS_VERSION(Game::EnvironmentGeneratorComponent::Layer::Object, 0);
CEREAL_CLASS_VERSION(Game::EnvironmentGeneratorComponent::Layer::NoiseInfluence, 0);

namespace cereal
{
	template<class Archive>
	void serialize(Archive& ar, Game::EnvironmentGeneratorComponent::Layer& value, uint32)
	{
		ar(value.mObjects, 
			value.mCellSize, 
			value.mMaxRandomOffset, 
			value.mNumberOfRandomRotations, 
			value.mNoiseScale, 
			value.mNoiseNumOfOctaves,
			value.mNoisePersistence,
			value.mInfluences,
			value.mWeight,
			value.mCanSpawnInOccupiedSpace);
	}

	template<class Archive>
	void serialize(Archive& ar, Game::EnvironmentGeneratorComponent::Layer::Object& value, uint32)
	{
		ar(value.mBaseFrequency, value.mPrefab, value.mScaleAtNoiseValue, value.mSpawnFrequenciesAtNoiseValue);
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