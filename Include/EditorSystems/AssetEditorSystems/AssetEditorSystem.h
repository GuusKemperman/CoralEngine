#pragma once
#ifdef EDITOR
#include "EditorSystems/EditorSystem.h"

#include <future>

#include "Utilities/Time.h"
#include "Assets/Asset.h"
#include "Assets/Core/AssetSaveInfo.h"
#include "Core/Editor.h"
#include "Meta/MetaType.h"

namespace CE
{
	class AssetEditorMementoStack
	{
	public:
		AssetEditorMementoStack() = default;
		AssetEditorMementoStack(const AssetEditorMementoStack&) = delete;
		AssetEditorMementoStack(AssetEditorMementoStack&&) noexcept = default;

		AssetEditorMementoStack& operator=(const AssetEditorMementoStack&) = delete;
		AssetEditorMementoStack& operator=(AssetEditorMementoStack&&) noexcept = default;

		~AssetEditorMementoStack() = default;

		struct SimilarityToFile
		{
			bool mDoesStateMatchFile{};
			std::filesystem::file_time_type mFileWriteTimeLastTimeWeChecked{};
		};

		struct Action
		{
			std::string mState{};
			bool mRequiresReserialization{};
			SimilarityToFile mSimilarityToFile{};
		};

		void Do(std::shared_ptr<Action> action);

		bool TryUndo();
		bool TryRedo();

		void ClearRedo();

		std::shared_ptr<Action> GetMostRecentState() const;

		std::vector<std::shared_ptr<Action>> mActions{};
		size_t mNumOfActionsDone{};
	};

	class AssetEditorSystemBase :
		public EditorSystem
	{
	public:
		AssetEditorSystemBase(const Asset& asset);

		~AssetEditorSystemBase();

		virtual const Asset& GetAsset() const = 0;
		virtual Asset& GetAsset() = 0;

		/*
		AssetEditorSystems generally work on a copy of the original asset,
		in order to prevent your changes leading to unexpected behaviour
		elsewhere. This function return the orginal asset, if it exists.
		*/
		WeakAssetHandle<Asset> TryGetOriginalAsset() const;

		std::optional<std::filesystem::path> GetDestinationFile() const;

		/*
		Saves the asset to file at the end of the frame. Will do a
		'restart' of the engine that is completely hidden from the
		users, but this restart makes sure your changes are correctly
		applied throughout the engine.
		*/
		void SaveToFile();

		/*
		Saves the asset to memory. The resulting AssetSaveInfo can
		be used to construct a copy of the asset.
		*/
		AssetSaveInfo SaveToMemory();

		bool IsSavedToFile() const;

		void Tick(float deltaTime) override;

		AssetEditorMementoStack ExtractStack();

		void InsertMementoStack(AssetEditorMementoStack stack);

	protected:
		bool Begin(ImGuiWindowFlags flags) override;

		void ShowSaveButton();

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AssetEditorSystemBase);

		/*
		Often the changes aren't directly made to the asset; for example you will be
		editing a world, moving some entities, deleting some others, but you are not
		directly modifing the asset, instead storing the changes inside of the system.

		This function alerts you that any changes you may have made must be applied to
		the asset now.
		*/
		virtual void ApplyChangesToAsset() {}

		virtual std::unique_ptr<Asset, InPlaceDeleter<Asset, true>> ConstructAsset(AssetLoadInfo& loadInfo) = 0;

		void CheckForDifferences();

		/*
		 * Some assets are different every time we load them. entt::registry for example may completely
		 * shuffle all the entities around as it wishes. So we load and save the asset to account for this.
		 */
		std::string Reserialize(std::string_view serialized);

		AssetEditorMementoStack mMementoStack{};

		Cooldown mDifferenceCheckCooldown{ 1.0f };

		struct CachedFile
		{
			std::filesystem::file_time_type mLastWriteTime{};
			std::string mFileContents{};
		};
		CachedFile mCachedFile{};

		std::future<std::optional<AssetEditorMementoStack::Action>> mActionToAdd{};
	};

	/*
	An EditorSystem specialized for editing assets. Deriving from this
	will link the typename to your derived AssetEditor class, which means
	that clicking on the Asset in the contentbrowser will create an instance
	of your system.
	*/
	template<typename T>
	class AssetEditorSystem :
		public AssetEditorSystemBase
	{
	public:
		AssetEditorSystem(T&& asset);
		~AssetEditorSystem() override = default;

		const T& GetAsset() const override { return mAsset; }
		T& GetAsset() override { return mAsset; }

		//********************************//
		//		Virtual functions		  //
		//********************************//

		void Tick(float deltaTime) override;

	protected:
		T mAsset;

	private:
		friend ReflectAccess;
		static MetaType Reflect();

		std::unique_ptr<Asset, InPlaceDeleter<Asset, true>> ConstructAsset(AssetLoadInfo& loadInfo) final override;
	};

	namespace Internal
	{
		std::string GetSystemNameBasedOnAssetName(std::string_view assetName);
		std::string GetAssetNameBasedOnSystemName(std::string_view systemName);
	}

	template<typename T>
	AssetEditorSystem<T>::AssetEditorSystem(T&& asset) :
		AssetEditorSystemBase(asset),
		mAsset(std::move(asset))
	{
	}

	template <typename T>
	void AssetEditorSystem<T>::Tick(float deltaTime)
	{
		AssetEditorSystemBase::Tick(deltaTime);
	}

	template <typename T>
	MetaType AssetEditorSystem<T>::Reflect()
	{
		// Bind the asset to our editor
		Editor::RegisterAssetEditorSystem<T>();
		return MetaType{ MetaType::T<AssetEditorSystem<T>>{}, Format("AssetEditorSystem<{}>", MakeTypeName<T>()), MetaType::Base<AssetEditorSystemBase>{} };
	}

	template <typename T>
	std::unique_ptr<Asset, InPlaceDeleter<Asset, true>> AssetEditorSystem<T>::ConstructAsset(AssetLoadInfo& loadInfo)
	{
		return MakeUniqueInPlace<T, Asset>(loadInfo);
	}
}

#endif // EDITOR