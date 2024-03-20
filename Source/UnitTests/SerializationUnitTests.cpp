#include "Precomp.h"

#include "Assets/Core/AssetLoadInfo.h"
#include "Core/UnitTests.h"
#include "Core/Editor.h"
#include "Core/AssetManager.h"
#include "Meta/MetaType.h"
#include "Containers/view_istream.h"
#include "World/Registry.h"
#include "World/World.h"

using namespace Engine;

UNIT_TEST(Serialization, AllAssetSerialization)
{
	std::vector<WeakAsset<Asset>> allAssets = AssetManager::Get().GetAllAssets();

	UnitTest::Result result = UnitTest::Success;

	// Reuse the same string to reduce memory allocations
	std::string savedAsset{};
	std::string resavedAsset{};

	uint32 bufferSize{};
	uint32 bufferAlign{};

	for (WeakAsset<Asset> asset : allAssets)
	{
		bufferSize = std::max(asset.GetAssetClass().GetSize(), bufferSize);
		bufferAlign = std::max(asset.GetAssetClass().GetAlignment(), bufferAlign);
	}

	TEST_ASSERT(bufferSize != 0 && bufferAlign != 0);

	void* assetBuffer = FastAlloc(bufferSize, bufferAlign);

	TEST_ASSERT(assetBuffer != nullptr);

	for (WeakAsset<Asset> asset : allAssets)
	{
		const MetaType& type = asset.GetAssetClass();

		if (!Editor::Get().IsThereAnEditorTypeForAssetType(type.GetTypeId()))
		{
			continue;
		}

		std::shared_ptr<const Asset> loadedAsset = asset.MakeShared();

		savedAsset = loadedAsset->Save().ToString();

		std::optional<AssetLoadInfo> loadInfo = AssetLoadInfo::LoadFromStream(std::make_unique<view_istream>(savedAsset));

		FuncResult reloadedAssetConstructResult = type.ConstructAt(assetBuffer, *loadInfo);

		if (reloadedAssetConstructResult.HasError())
		{
			LOG(LogUnitTests, Error, "Failed to construct asset {} of type {} with a loadInfo object - {}", asset.GetName(), type.GetName(), reloadedAssetConstructResult.Error());
			result = UnitTest::Failure;
			continue;
		}

		const Asset* const reloadedAsset = reloadedAssetConstructResult.GetReturnValue().As<Asset>();

		if (reloadedAsset == nullptr)
		{
			LOG(LogUnitTests, Error, "Construct asset {} of type {} returned an object that, according to MetaAny, is not an asset.", asset.GetName(), type.GetName());
			result = UnitTest::Failure;
			continue;
		}

		resavedAsset = reloadedAsset->Save().ToString();

		if (savedAsset == resavedAsset)
		{
			continue;
		}

		LOG(LogUnitTests, Error, "Asset {} of type {} produced a different save after reloading. Serialization is likely broken", asset.GetName(), type.GetName());
		result = UnitTest::Failure;
	}

	_aligned_free(assetBuffer);

	return result;
}