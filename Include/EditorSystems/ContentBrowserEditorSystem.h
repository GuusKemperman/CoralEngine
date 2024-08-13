#ifdef EDITOR
#pragma once
#include <future>

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
		~ContentBrowserEditorSystem();

		void Tick(float deltaTime) override;

		void SaveState(std::ostream& toStream) const override;

		void LoadState(std::istream& fromStream) override;

	private:
		struct ContentFolder
		{
			ContentFolder(const std::filesystem::path& path, std::string&& name, ContentFolder* parent) :
				mActualPath(path),
				mFolderName(std::move(name)),
				mParent(parent)
			{}

			ContentFolder(const ContentFolder&) = delete;
			ContentFolder(ContentFolder&&) noexcept = delete;

			ContentFolder& operator=(const ContentFolder&) = delete;
			ContentFolder& operator=(ContentFolder&&) noexcept = delete;

			// Will be empty for the root folder
			std::filesystem::path mActualPath{};
			std::string mFolderName{};

			std::list<ContentFolder> mChildren{};
			ContentFolder* mParent{};
			std::vector<WeakAssetHandle<>> mContent{};
		};

		void RequestUpdateToFolderGraph();

		void DisplayFolderHierarchyPanel();
		void DisplayContentPanel();

		void DisplayFolder(const ContentFolder& folder);

		template<typename T>
		void DisplayItemInFolder(T& item, ThumbnailEditorSystem& thumbnailSystem);

		static void OpenAsset(WeakAssetHandle<> asset);

		static bool DisplayAssetNameUI(std::string& name);
		void DisplayCreateNewAssetMenu(const ContentFolder& inFolder);

		static void PushError();
		static void PopError();

		static void Reimport(const WeakAssetHandle<>& asset);

		static std::string_view GetName(const WeakAssetHandle<>& asset);

		static std::string_view GetName(const ContentFolder& folder);

		void DisplayPathToCurrentlySelectedFolder(const ContentFolder& folder);

		void DisplayImage(const WeakAssetHandle<>& asset, ThumbnailEditorSystem& thumbnailSystem);
		void DisplayImage(const ContentFolder& assetfolder, ThumbnailEditorSystem& thumbnailSystem);

		static std::string GetRightClickPopUpMenuName(std::string_view itemName);

		void DisplayRightClickMenu(const WeakAssetHandle<>& asset);
		void DisplayRightClickMenu(const ContentFolder& folder);

		void SelectFolder(const ContentFolder& folder);
		void SelectFolder(const std::filesystem::path& path);

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ContentBrowserEditorSystem);

		std::unique_ptr<ContentFolder> mRootFolder = std::make_unique<ContentFolder>(std::filesystem::path{}, "All", nullptr);
		std::reference_wrapper<const ContentFolder> mSelectedFolder = *mRootFolder;
		std::filesystem::path mSelectedFolderPath{};

		std::future<std::unique_ptr<ContentFolder>> mPendingRootFolder{};

		float mFolderHierarchyPanelWidthPercentage = .25f;
		float mContentPanelWidthPercentage = .75f;
	};
}
#endif // EDITOR