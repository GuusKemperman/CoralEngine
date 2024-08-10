#pragma once
#include "Assets/Core/AssetHandle.h"

namespace CE
{
	class Prefab;
	class World;

	class GridSpawnerComponent
	{
	public:
		void OnConstruct(World&, entt::entity owner);
		void OnBeginPlay(World&, entt::entity);

		void ClearGrid();
		void SpawnGrid();

		std::vector<AssetHandle<Prefab>> mTiles{};

		glm::vec2 mSpacing{ 1.0f };
		uint32 mWidth = 1;
		uint32 mHeight = 1;

		uint32 mNumOfPossibleRotations = 1;
		float mMaxRandomOffset{};
		float mMinRandomHeightOffset{};
		float mMaxRandomHeightOffset{};

		float mMinRandomScale{ 1.0f };
		float mMaxRandomScale{ 1.0f };

		glm::vec2 mMinRandomOrientation{};
		glm::vec2 mMaxRandomOrientation{};

		bool mShouldSpawnOnBeginPlay{};
		bool mIsCentered{};

		entt::entity mOwner{};

		bool mUseWorldPositionAsSeed = true;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(GridSpawnerComponent);
	};
}
