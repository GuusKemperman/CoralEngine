#ifdef EDITOR
#pragma once
#include "EditorSystems/EditorSystem.h"

#include "Core/AssetManager.h"

namespace Engine
{
	class MetaType;

	class ContentBrowserEditorSystem final :
		public EditorSystem
	{
	public:
		ContentBrowserEditorSystem();

		void Tick(float deltaTime) override;

		struct ContentFolder
		{
			ContentFolder() = default;
			ContentFolder(const std::filesystem::path& path) : mPath(path) {}
			std::filesystem::path mPath{};
			std::vector<ContentFolder> mChildren{};
			std::vector<WeakAsset<Asset>> mContent{};
		};
		static std::vector<ContentFolder> MakeFolderGraph(std::vector<WeakAsset<Asset>>&& assets);

	private:
		// Moved in because the assets may be deleted, don't hold onto them.
		void DisplayDirectory(ContentFolder&& folder);

		// Moved in because the asset may be deleted, don't hold onto it.
		void DisplayAsset(WeakAsset<Asset>&& asset) const;

		void OpenAsset(WeakAsset<Asset> asset) const;

		struct AssetCreator
		{
			const MetaType* mClass{};
			std::string mAssetName{};
			std::string mFolderRelativeToRoot{};
		};
		std::optional<AssetCreator> mAssetCreator{};
		void DisplayAssetCreator(const std::vector<ContentFolder>& contentFolders);

		static void CreateNewAsset(const AssetCreator& assetCreator, const std::filesystem::path& toFile);

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ContentBrowserEditorSystem);
	};
}
#endif // EDITOR