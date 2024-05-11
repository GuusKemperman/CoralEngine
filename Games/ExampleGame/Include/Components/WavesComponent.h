#pragma once
#include "Assets/Prefabs/Prefab.h"
#include "Meta/MetaType.h"

#include "Assets/Asset.h"
#include "Assets/Core/AssetHandle.h"

namespace Game
{
	struct EnemyTypeAndCount
	{
		CE::AssetHandle<CE::Prefab> mEnemy{};

		int mAmount{};

		bool operator==(const EnemyTypeAndCount& other) const;
		bool operator!=(const EnemyTypeAndCount& other) const;

#ifdef EDITOR
		void DisplayWidget();
#endif // EDITOR

	private:
		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(EnemyTypeAndCount);
	};

	class WavesComponent
	{
	public:
		std::vector<std::vector<EnemyTypeAndCount>> mWaves{};

	private:
		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(WavesComponent);
	};

	template <class Archive>
	void serialize([[maybe_unused]] Archive& ar, [[maybe_unused]] EnemyTypeAndCount& value)
	{
		ar(value.mEnemy, value.mAmount);
	}
}

#ifdef EDITOR
IMGUI_AUTO_DEFINE_INLINE(template<>, Game::EnemyTypeAndCount, var.DisplayWidget(); (void)name;)
#endif // EDITOR
