#ifdef EDITOR
#pragma once
#include "EditorSystems/EditorSystem.h"

#include "Core/AssetManager.h"

namespace CE
{
	class MetaType;

	class ContentBrowserEditorSystem final :
		public EditorSystem
	{
	public:
		ContentBrowserEditorSystem();

		void Tick(float deltaTime) override;


	private:
		struct ContentFolder
		{
			ContentFolder() = default;
			ContentFolder(const std::filesystem::path& path) : mPath(path) {}
			std::filesystem::path mPath{};
			std::vector<ContentFolder> mChildren{};
			std::vector<WeakAsset<Asset>> mContent{};
		};
		static std::vector<ContentFolder> MakeFolderGraph(std::vector<WeakAsset<Asset>>&& assets);

		// Moved in because the assets may be deleted, don't hold onto them.
		void DisplayDirectory(const ContentFolder& folder);

		// Moved in because the asset may be deleted, don't hold onto it.
		void DisplayAsset(const WeakAsset<Asset>& asset) const;

		void OpenAsset(WeakAsset<Asset> asset) const;

		static bool DisplayNameUI(std::string& name);

		struct FilePathUIResult
		{
			std::filesystem::path mActualFullPath{};
			std::filesystem::path mPathToShowUser{};
			bool mAnyErrors{};
		};

		static FilePathUIResult DisplayFilepathUI(std::string& folderRelativeToRoot, bool& isEngineAsset, const std::string& assetName);

		static void PushError();
		static void PopError();

		void DisplayAssetCreatorPopUp();
		void DisplayAssetRightClickPopUp();

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ContentBrowserEditorSystem);

		std::vector<ContentFolder> mFolderGraph{};
	};
}
#endif // EDITOR