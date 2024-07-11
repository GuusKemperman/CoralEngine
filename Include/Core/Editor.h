#ifdef EDITOR
#pragma once
#include "Core/EngineSubsystem.h"

#include <forward_list>

#include "Core/AssetManager.h"
#include "EditorSystems/EditorSystem.h"
#include "Meta/MetaTypeId.h"

namespace CE
{
	class AssetLoadInfo;

	class Editor final :
		public EngineSubsystem<Editor>
	{
		friend EngineSubsystem;
		void PostConstruct() final override;
		~Editor() final override;

	public:
		void Tick(float deltaTime);

		/*
		Get a system by name.

		Note:
			If there is no system with this name, this function will return nullptr.
			If the system exists but is not of type CastTo, this function will return nullptr.
		*/
		template<typename CastTo = EditorSystem>
		CastTo* TryGetSystem(std::string_view systemName);

		/*
		Get a system by type.

		Note:
			If there is no system of this type, this function will return nullptr.
		*/
		template<typename SystemType>
		SystemType* TryGetSystem();

		/*
		Add a system. The arguments provided will be passed to the constructor of T.

		If there is already a system with the same name, this function will emit a warning
		and return nullptr.
		*/
		template<typename T, typename ...Args>
		T* TryAddEditorSystem(Args&& ...args);

		/*
		Note:
			The windowgroup will not actually be destroyed until the end of this frame,
			but destruction of all requested windows can be forced using DestroyRequestedSystems
		*/
		void DestroySystem(std::string_view systemName);

		struct RefreshRequest
		{
			enum Flags
			{
				SaveAssetsToFile = 1 << 1,
				ReloadAssets = 1 << 2,
				ReloadOtherSystems = 1 << 3,
				Volatile = ReloadAssets | ReloadOtherSystems
			};
			uint32 mFlags{};
			std::function<void()> mActionWhileUnloaded{};

			// May be left empty if ReloadOtherSystems
			std::string mNameOfSystemToRefresh{};
		};

		/*
		If you are making an edit that can have drastic side-effects on other editor windows,
		such as recompiling scripts or saving an asset that is referenced by the other windows,
		this function can be used.

		It will wait until the end of the frame. Depending on the settings, it will deconstruct
		all the editor windows, commit the action, and then restore them.
		*/
		void Refresh(RefreshRequest&& refreshMethod);

		/**
		 * \brief A dangerous function, do not call unless you know what you're doing.
		 *
		 * Will destroy and recreate pretty much everything that you have open on screen.
		 * Calling this from a system that is owned by the editor will lead to undefined behaviour.
		 *
		 */
		void FullFillRefreshRequests();

		/*
		Saves all the editors currently being edited to file.
		*/
		void SaveAll();

		/*
		The main way of opening an asset for edit. If the asset is already open
		for edit, the existing system is returned.

		Note:
			The original asset will not be modified until the user saves 
			their changes.

		Returns:
			Will return nullptr if there is already a window with the asset name.
			Will return nullptr if there is no editor for this type of asset.
		*/
		EditorSystem* TryOpenAssetForEdit(const WeakAssetHandle<>& originalAsset);

		// Checks if there is any editor system that could open an asset of this type
		// for edit.
		bool IsThereAnEditorTypeForAssetType(TypeId assetTypeId) const;

	private:
		EditorSystem* TryOpenAssetForEdit(AssetLoadInfo&& loadInfo);

		/*
		We use the meta runtime reflection system to create some of our systems.
		This requires using placement new and thus a custom deleter.
		*/
		template<typename T>
		using SystemPtr = std::unique_ptr<T, InPlaceDeleter<T, true>>;

		/*
		Is automatically called at the end of Tick. Use with caution, 
		do not call from a windowgroup awaiting destruction.
		*/
		void DestroyRequestedSystems();

		/*
		The directory to which all the system's states are saved to when 
		the engine is shut down. When we open the engine again, we load
		the states from this directory.
		*/
		static std::filesystem::path GetFileLocationOfSystemState(std::string_view systemName);

		void LoadSystemState(EditorSystem& system);
		
		void SaveSystemState(const EditorSystem& system);

		EditorSystem* TryAddSystemInternal(TypeId typeId, SystemPtr<EditorSystem> system);

		/*
		Will construct the type and load it's saved state. Will fail if the type is not default constructible or not a type.
		*/
		EditorSystem* TryAddSystemInternal(const MetaType& type);

		EditorSystem* TryGetSystemInternal(std::string_view systemName);

		/*
		Only returns true if the types exactly match; no checks are done for base/derive logic.
		Maybe if it handles that logic as well, it could be exposed in the public API.
		*/
		EditorSystem* TryGetSystemInternal(TypeId typeId);

		void DisplayMainMenuBar();

		template<typename T>
		friend class AssetEditorSystem;

		static std::unordered_map<TypeId, TypeId>& GetAssetKeyAssetEditorPairs()
		{
			static std::unordered_map<TypeId, TypeId> instance{};
			return instance;
		}

		template<typename AssetT>
		static void RegisterAssetEditorSystem()
		{
			[[maybe_unused]] const auto result = GetAssetKeyAssetEditorPairs().emplace(
				MakeTypeId<AssetT>(), 
				MakeTypeId<AssetEditorSystem<AssetT>>());
			ASSERT(result.second);
		}

		std::vector<std::pair<TypeId, SystemPtr<EditorSystem>>> mSystems{};
		std::forward_list<std::string> mSystemsToDestroy{};

		std::vector<RefreshRequest> mRefreshRequests{};
		std::chrono::system_clock::time_point mTimeOfLastRefresh = std::chrono::system_clock::now();
	};

	template<typename CastTo>
	CastTo* Editor::TryGetSystem(const std::string_view systemName)
	{
		return dynamic_cast<CastTo*>(TryGetSystemInternal(systemName));
	}

	template <typename SystemType>
	SystemType* Editor::TryGetSystem()
	{
		return dynamic_cast<SystemType*>(TryGetSystemInternal(MakeTypeId<SystemType>()));
	}

	template<typename T, typename ...Args>
	T* Editor::TryAddEditorSystem(Args && ...args)
	{
		return static_cast<T*>(TryAddSystemInternal(MakeTypeId<T>(), MakeUniqueInPlace<T>(std::forward<Args>(args)...)));
	}
}
#endif // EDITOR