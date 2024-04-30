#ifdef EDITOR
#pragma once
#include "EditorSystems/EditorSystem.h"
#include "Core/AssetManager.h"

namespace CE
{
	class MetaType;
	class ThumbnailEditorSystem;

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

			ContentFolder(const ContentFolder&) = delete;
			ContentFolder(ContentFolder&&) noexcept = default;

			ContentFolder& operator=(const ContentFolder&) = delete;
			ContentFolder& operator=(ContentFolder&&) noexcept = default;

			std::filesystem::path mPath{};
			std::vector<ContentFolder> mChildren{};
			std::vector<WeakAssetHandle<>> mContent{};
		};
		static std::vector<ContentFolder> MakeFolderGraph();

		void DisplayFolder(ContentFolder& folder, ThumbnailEditorSystem& thumbnailSystem);

		void DisplayAsset(WeakAssetHandle<>& asset, ThumbnailEditorSystem& thumbnailSystem) const;

		void OpenAsset(WeakAssetHandle<> asset) const;

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

		static void Reimport(const WeakAssetHandle<>& asset);

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ContentBrowserEditorSystem);

		std::vector<ContentFolder> mFolderGraph{};
	};
}
#endif // EDITOR