#ifdef EDITOR
#pragma once
#include "EditorSystems/EditorSystem.h"

#include "Core/AssetManager.h"

namespace CE
{
	class Texture;
	class MetaType;

	class ContentBrowserEditorSystem final :
		public EditorSystem
	{
	public:
		ContentBrowserEditorSystem();

		void Tick(float deltaTime) override;

	private:
		struct ContentEntry
		{
			WeakAssetHandle<> mAsset{};

			// Will be nullopt if there has not yet been
			// an attempt to generate a thumbnail.
			// May still be nullptr if the attempt was made,
			// but failed.
			std::optional<AssetHandle<Texture>> mThumbnail{};
		};

		struct ContentFolder
		{
			ContentFolder() = default;
			ContentFolder(const std::filesystem::path& path) : mPath(path) {}
			std::filesystem::path mPath{};
			std::vector<ContentFolder> mChildren{};
			std::vector<ContentEntry> mContent{};
		};
		static std::vector<ContentFolder> MakeFolderGraph();

		void DisplayDirectory(ContentFolder& folder);

		void DisplayEntry(ContentEntry& entry) const;

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