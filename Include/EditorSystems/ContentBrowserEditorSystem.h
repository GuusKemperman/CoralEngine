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
		static ContentFolder MakeFolderGraph();

		void DisplayFolder(ContentFolder& folder);

		template<typename T>
		void DisplayItemInFolder(T& item, ThumbnailEditorSystem& thumbnailSystem);

		static void OpenAsset(WeakAssetHandle<> asset);

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

		static void Reimport(const WeakAssetHandle<>& asset);

		static std::string_view GetName(const WeakAssetHandle<>& asset);

		static std::string GetName(const ContentFolder& folder);

		void DisplayImage(const WeakAssetHandle<>& asset, ThumbnailEditorSystem& thumbnailSystem);
		void DisplayImage(ContentFolder& assetfolder, ThumbnailEditorSystem& thumbnailSystem);

		static std::string GetRightClickPopUpMenuName(std::string_view itemName);

		void DisplayRightClickMenu(const WeakAssetHandle<>& asset);
		void DisplayRightClickMenu(const ContentFolder& folder);

		void ShowCreateNewMenu();

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ContentBrowserEditorSystem);

		ContentFolder mFolderGraph{};
		ContentFolder* mSelectedFolder{};

		float mFolderHierarchyPanelWidthPercentage = .25f;
		float mContentPanelWidthPercentage = .75f;
	};
}
#endif // EDITOR