#pragma once
#include "Assets/Core/AssetHandle.h"
#include "Meta/MetaReflect.h"
#include "Utilities/WeightedRandomDistribution.h"

namespace CE
{
	class Prefab;
}

namespace Game
{
	class SpawnerComponent
	{
	public:
		// The range in which the player has to be for the spawner to be active.
		float mMinSpawnRange = 100.0f;

		float mMaxEnemyDistance = 500.0f;

		float mSpacing = 5.0f;

		struct Wave
		{
			struct EnemyType
			{
				bool operator!=(const EnemyType& enemy) const;
				bool operator==(const EnemyType& enemy) const;

				CE::AssetHandle<CE::Prefab> mPrefab{};
				float mSpawnChance{};

				std::optional<uint32> mMaxAmountAlive{};
				std::optional<uint32> mAmountToSpawnAtStartOfWave{};

#ifdef EDITOR
				void DisplayWidget(const std::string& name);
#endif // EDITOR

			private:
				friend CE::ReflectAccess;
				static CE::MetaType Reflect();
			};

			bool operator!=(const Wave& wave) const;
			bool operator==(const Wave& wave) const;

#ifdef EDITOR
			void DisplayWidget(const std::string& name);
#endif // EDITOR

			std::vector<EnemyType> mEnemies{};

			float mDuration{};
			float mAmountToSpawnPerSecond{};
			float mAmountToSpawnPerSecondWhenBelowMinimum{};
			uint32 mDesiredMinimumNumberOfEnemies{};

		private:
			friend CE::ReflectAccess;
			static CE::MetaType Reflect();
		};
		std::vector<Wave> mWaves{};

	private:
		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(SpawnerComponent);
	};
}

CEREAL_CLASS_VERSION(Game::SpawnerComponent::Wave, 0);
CEREAL_CLASS_VERSION(Game::SpawnerComponent::Wave::EnemyType, 0);

namespace cereal
{
	template<class Archive>
	void serialize(Archive& ar, Game::SpawnerComponent::Wave::EnemyType& value, uint32)
	{
		ar(value.mMaxAmountAlive, value.mAmountToSpawnAtStartOfWave, value.mPrefab, value.mSpawnChance);
	}

	template<class Archive>
	void serialize(Archive& ar, Game::SpawnerComponent::Wave& value, uint32)
	{
		ar(value.mAmountToSpawnPerSecond, value.mAmountToSpawnPerSecondWhenBelowMinimum, value.mDuration, value.mEnemies);
	}
}

#ifdef EDITOR
IMGUI_AUTO_DEFINE_INLINE(template<>, Game::SpawnerComponent::Wave::EnemyType, var.DisplayWidget(name);)
IMGUI_AUTO_DEFINE_INLINE(template<>, Game::SpawnerComponent::Wave, var.DisplayWidget(name);)
#endif // EDITOR
