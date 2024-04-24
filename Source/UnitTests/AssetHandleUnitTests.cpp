#include "Precomp.h"
#include "Assets/Core/AssetHandle.h"

#include "Core/UnitTests.h"
#include "Meta/MetaManager.h"
#include "Assets/StaticMesh.h"
#include "Assets/Prefabs/Prefab.h"

using namespace CE;

UNIT_TEST(AssetHandleTests, SingleThread)
{
	{
		const AssetHandle<StaticMesh> emptyHandle{};
		TEST_ASSERT(emptyHandle.GetNumberOfStrongReferences() == 0);
	}

	Internal::AssetInternal assetInternal{ AssetFileMetaData{"TestAsset", MetaManager::Get().GetType<StaticMesh>() }, std::nullopt };

	{
		AssetHandle<StaticMesh> nonEmpty{ &assetInternal };

		TEST_ASSERT(nonEmpty.GetNumberOfStrongReferences() == 1);

		{
			AssetHandle<> nonEmpty2{ &assetInternal };
			TEST_ASSERT(nonEmpty.GetNumberOfStrongReferences() == 2);

			AssetHandle<StaticMesh> nonEmpty3{};
			TEST_ASSERT(nonEmpty.GetNumberOfStrongReferences() == 2);
			nonEmpty3 = StaticAssetHandleCast<StaticMesh>(nonEmpty2);
			TEST_ASSERT(nonEmpty.GetNumberOfStrongReferences() == 3);

			AssetHandle<> nonEmpty4{ std::move(nonEmpty3) };
			TEST_ASSERT(nonEmpty.GetNumberOfStrongReferences() == 3);

			AssetHandle<> nonEmpty5{};
			nonEmpty5 = std::move(nonEmpty4);
			TEST_ASSERT(nonEmpty.GetNumberOfStrongReferences() == 3);

			nonEmpty5 = nullptr;
			TEST_ASSERT(nonEmpty.GetNumberOfStrongReferences() == 2);

		}
		TEST_ASSERT(nonEmpty.GetNumberOfStrongReferences() == 1);
	}
	TEST_ASSERT(assetInternal.mRefCounters[static_cast<int>(Internal::AssetInternal::RefCountType::Strong)] == 0);

	return UnitTest::Success;
}


UNIT_TEST(AssetHandleTests, DynamicCasts)
{
	{
		const AssetHandle<StaticMesh> emptyHandle{};
		TEST_ASSERT(emptyHandle.GetNumberOfStrongReferences() == 0);
	}

	Internal::AssetInternal assetInternal{ AssetFileMetaData{"TestAsset", MetaManager::Get().GetType<StaticMesh>() }, std::nullopt };

	AssetHandle<StaticMesh> derived{ &assetInternal };
	AssetHandle<> base = derived;
	TEST_ASSERT(base == derived);

	TEST_ASSERT(DynamicAssetHandleCast<StaticMesh>(base) != nullptr);
	TEST_ASSERT(DynamicAssetHandleCast<Prefab>(base) == nullptr);

	TEST_ASSERT(DynamicAssetHandleCast<StaticMesh>(base) == derived);
	TEST_ASSERT(DynamicAssetHandleCast<Prefab>(base) != derived);

	return UnitTest::Success;
}
