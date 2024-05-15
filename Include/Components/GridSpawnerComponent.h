#pragma once
#include "Assets/Core/AssetHandle.h"

namespace CE
{
	class Prefab;
	class World;

	class GridSpawner
	{
	public:
		void OnConstruct(World&, entt::entity owner);

		void SpawnGrid();

		std::vector<AssetHandle<Prefab>> mTiles{};
		std::vector<float> mSpawnChances{ 1.0f };

		glm::vec2 mSpacing{ 1.0f };
		uint32 mWidth = 1;
		uint32 mHeight = 1;

		bool mIsCentered{};

		entt::entity mOwner{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(GridSpawner);
	};
}
