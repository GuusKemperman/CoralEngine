#include "Precomp.h"

#include "Assets/Core/AssetLoadInfo.h"
#include "Assets/Core/AssetSaveInfo.h"
#include "Core/UnitTests.h"
#include "Core/Editor.h"
#include "Core/AssetManager.h"
#include "Meta/MetaType.h"
#include "Utilities/view_istream.h"
#include "World/Registry.h"
#include "World/World.h"

using namespace CE;

//UNI_TEST(Serialization, AllAssetSerialization)
//{
//#ifdef EDITOR
//	std::vector<WeakAssetHandle<>> allAssets = AssetManager::Get().GetAllAssets();
//
//	UnitTest::Result result = UnitTest::Success;
//
//	// Reuse the same string to reduce memory allocations
//	std::string savedAsset{};
//	std::string resavedAsset{};
//
//	uint32 bufferSize{};
//	uint32 bufferAlign{};
//
//	for (WeakAssetHandle<> asset : allAssets)
//	{
//		bufferSize = std::max(asset.GetMetaData().GetClass().GetSize(), bufferSize);
//		bufferAlign = std::max(asset.GetMetaData().GetClass().GetAlignment(), bufferAlign);
//	}
//
//	TEST_ASSERT(bufferSize != 0 && bufferAlign != 0);
//
//	void* assetBuffer = FastAlloc(bufferSize, bufferAlign);
//
//	TEST_ASSERT(assetBuffer != nullptr);
//
//	for (WeakAssetHandle<> asset : allAssets)
//	{
//		const MetaType& type = asset.GetMetaData().GetClass();
//
//		if (!Editor::Get().IsThereAnEditorTypeForAssetType(type.GetTypeId()))
//		{
//			continue;
//		}
//
//		AssetHandle<> loadedAsset = asset.MakeShared();
//
//		TEST_ASSERT(loadedAsset != nullptr);
//
//		savedAsset = loadedAsset->Save().ToString();
//
//		std::optional<AssetLoadInfo> loadInfo = AssetLoadInfo::LoadFromStream(std::make_unique<view_istream>(savedAsset));
//
//		TEST_ASSERT(loadInfo.has_value());
//
//		FuncResult reloadedAssetConstructResult = type.ConstructAt(assetBuffer, *loadInfo);
//
//		if (reloadedAssetConstructResult.HasError())
//		{
//			LOG(LogUnitTests, Error, "Failed to construct asset {} of type {} with a loadInfo object - {}", asset.GetMetaData().GetName(), type.GetMetaData().GetName(), reloadedAssetConstructResult.Error());
//			result = UnitTest::Failure;
//			continue;
//		}
//
//		const Asset* const reloadedAsset = reloadedAssetConstructResult.GetReturnValue().As<Asset>();
//
//		if (reloadedAsset == nullptr)
//		{
//			LOG(LogUnitTests, Error, "Construct asset {} of type {} returned an object that, according to MetaAny, is not an asset.", asset.GetMetaData().GetName(), type.GetMetaData().GetName());
//			result = UnitTest::Failure;
//			continue;
//		}
//
//		resavedAsset = reloadedAsset->Save().ToString();
//
//		if (savedAsset == resavedAsset)
//		{
//			continue;
//		}
//
//		LOG(LogUnitTests, Error, "Asset {} of type {} produced a different save after reloading. Serialization is likely broken", asset.GetMetaData().GetName(), type.GetMetaData().GetName());
//		result = UnitTest::Failure;
//	}
//
//	_aligned_free(assetBuffer);
//	return result;
//#else
//	return UnitTest::Success;
//#endif
//}