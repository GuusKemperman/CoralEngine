#ifdef EDITOR
#pragma once
#include "EditorSystems/EditorSystem.h"

#include "Assets/Asset.h"
#include "Assets/Core/AssetLoadInfo.h"
#include "Core/AssetManager.h"
#include "Core/Editor.h"
#include "Core/Input.h"
#include "Meta/MetaType.h"
#include "Containers/view_istream.h"
#include "Utilities/DoUndo.h"
#include "Utilities/StringFunctions.h"

namespace Engine
{
	/*
	Do not derive from this, derive from the templated class below.
	*/
	class AssetEditorSystemInterface
	{
	public:

		/*
		For documentation on any of these functions, see the overriden functions in
		AssetEditorSystem<T>		
		*/

		virtual AssetSaveInfo SaveToMemory() = 0;
		virtual void SaveToFile() = 0;
		virtual TypeId GetAssetTypeId() const = 0;

	protected:
		friend class Editor;
		struct MementoAction
		{
			MementoAction(std::string&& str, std::string_view nameOfAssetEditor) : mState(std::move(str)), mNameOfAssetEditor(nameOfAssetEditor) {}

			void Do();
			void Undo();
			void RefreshTheAssetEditor();

			std::string mState{};
			bool mDoIsNeeded{};

			std::string mNameOfAssetEditor{};

			// Mutable because the result is cached, but it does not influence the state
			// of the asset.
			mutable bool mIsSameAsFile{};
			mutable std::filesystem::file_time_type mTimeWeCheckedIfIsSameAsFile{};
		};

		using MementoStack = DoUndo::DoUndoStackBase<MementoAction>;

		void SetMementoStack(MementoStack&& stack) { mMementoStack = std::move(stack); };
		virtual MementoStack&& ExtractMementoStack() = 0;

		MementoStack mMementoStack{};
	};


	/*
	An EditorSystem specialized for editing assets. Deriving from this
	will link the typename to your derived AssetEditor class, which means
	that clicking on the Asset in the contentbrowser will create an instance
	of your system.
	*/
	template<typename T>
	class AssetEditorSystem :
		public EditorSystem,
		public AssetEditorSystemInterface
	{
	public:
		AssetEditorSystem(T&& asset);
		~AssetEditorSystem() override = default;

		const T& GetAsset() const { return mAsset; }
		T& GetAsset() { return mAsset; }

		/*
		AssetEditorSystems generally work on a copy of the original asset,
		in order to prevent your changes leading to unexpected behaviour
		elsewhere. This function return the orginal asset, if it exists.
		*/
		std::optional<WeakAsset<T>> TryGetOriginalAsset() const;

		/*
		Saves the asset to file at the end of the frame. Will do a 
		'restart' of the engine that is completely hidden from the
		users, but this restart makes sure your changes are correctly
		applied throughout the engine.

		If no file location is provided, the file location of the
		original asset will be used.
		*/
		void SaveToFile() final;

		/*
		Saves the asset to memory. The resulting AssetSaveInfo can
		be used to construct a copy of the asset. 
		
		Note that calling AssetSaveInfo::SaveToFile will not trigger 
		the engine to restart, which means your changes may not be 
		applied correctly, if at all. Prefer AssetEditorSystem::SaveToFile
		if you intend to save this asset to a file.
		*/
		[[nodiscard]] AssetSaveInfo SaveToMemory() final;

		bool IsSavedToFile() const;

		//********************************//
		//		Virtual functions		  //
		//********************************//

		void Tick(float deltaTime) override;

	protected:
		bool Begin(ImGuiWindowFlags flags) override;

	private:
		/*
		Often the changes aren't directly made to the asset; for example you will be 
		editing a world, moving some entities, deleting some others, but you are not 
		directly modifing the asset, instead storing the changes inside of the system.
		
		This function alerts you that any changes you may have made must be applied to
		the asset now. 
		*/
		virtual void ApplyChangesToAsset() {};

	protected:
		void ShowSaveButton();

		T mAsset;

		std::filesystem::path mPathToSaveAssetTo{};

	private:
		TypeId GetAssetTypeId() const final { return MakeTypeId<T>(); };

		friend ReflectAccess;
		static MetaType Reflect()
		{
			// Bind the asset to our editor
			Editor::RegisterAssetEditorSystem<T>();
			return MetaType{ MetaType::T<AssetEditorSystem<T>>{}, Format("AssetEditorSystem<{}>", MakeTypeName<T>()), MetaType::Base<EditorSystem>{} };
		}

		/*
		 * Checks if any changes have been made to our asset, and saves them to the do/undo stack.
		 * This allows each action to be undone by reverting to an older version.
		 *
		 * Is by default called on a cooldown, there is generally no need to call this yourself.
		 */
		void CheckForDifferences();

		MementoStack&& ExtractMementoStack() override;

		/*
		 * Some assets are different every time we load them. entt::registry for example may completely
		 * shuffle all the entities around as it wishes. So we load and save the asset to account for this.
		 */
		static std::string Reserialize(std::string_view serialized);

		static constexpr float sDifferenceCheckCoolDown = 4.0f;
		float mTimeLeftUntilCheckForDifference{};

		// Used for checking if our asset has unsaved changes. Kept in memory for performance reasons.
		// May not always be up to date, it's updated when needed.
		struct AssetOnFile
		{
			std::string mReserializedAsset{};

			// If changes are made to the asset file, this mAssetAsSeenOnFile is updated.
			std::filesystem::file_time_type mWriteTimeAtTimeOfReserializing{};
		};
		mutable AssetOnFile mAssetOnFile{};
	};

	namespace Internal
	{
		std::string GetSystemNameBasedOnAssetName(std::string_view assetName);
		std::string GetAssetNameBasedOnSystemName(std::string_view systemName);
	}

	inline void AssetEditorSystemInterface::MementoAction::Do()
	{
		if (!mDoIsNeeded)
		{
			return;
		}

		RefreshTheAssetEditor();
	}

	inline void AssetEditorSystemInterface::MementoAction::Undo()
	{
		mDoIsNeeded = true;
		RefreshTheAssetEditor();
	}

	inline void AssetEditorSystemInterface::MementoAction::RefreshTheAssetEditor()
	{
		// The refreshing will return the asset editor to the
		// top state in the stack
		Editor::Get().Refresh({ 0, {}, mNameOfAssetEditor });
	}

	template<typename T>
	AssetEditorSystem<T>::AssetEditorSystem(T&& asset) :
		EditorSystem(Internal::GetSystemNameBasedOnAssetName(asset.GetName())),
		mAsset(std::move(asset)),
		mPathToSaveAssetTo([this]() -> std::filesystem::path
			{
				std::optional<WeakAsset<T>> originalAsset = TryGetOriginalAsset();

				if (originalAsset.has_value())
				{
					return originalAsset->GetFileOfOrigin().value_or(std::filesystem::path{});
				}
				return {};
			}
			())
	{
	}

	template<typename T>
	std::optional<WeakAsset<T>> AssetEditorSystem<T>::TryGetOriginalAsset() const
	{
		return AssetManager::Get().TryGetWeakAsset<T>(mAsset.GetName());
	}

	template<typename T>
	void AssetEditorSystem<T>::SaveToFile()
	{
		if (mPathToSaveAssetTo.empty())
		{
			LOG(LogEditor, Warning, "Could not save asset {} - The file location was empty. This is caused by there not being an original asset, and mPathToSaveAssetTo was never updated.",
				mAsset.GetName());
			return;
		}

		Editor::Get().Refresh(
			{
				Editor::RefreshRequest::Volatile,

				// A shared pointer, because it makes move semantics with lambdas so much simpler
				[assetSaveInfo = std::make_shared<AssetSaveInfo>(SaveToMemory()), saveTo = mPathToSaveAssetTo]
				{
					assetSaveInfo->SaveToFile(saveTo);
				}
			});
	}

	template<typename T>
	AssetSaveInfo AssetEditorSystem<T>::SaveToMemory()
	{
		ApplyChangesToAsset();
		const std::optional<WeakAsset<T>> originalAsset = TryGetOriginalAsset();

		AssetSaveInfo saveInfo = mAsset.Save(originalAsset->GetImportedFromFile().has_value() 
			? AssetFileMetaData::ImporterInfo{ *originalAsset->GetImportedFromFile(), *originalAsset->GetImporterVersion()}
			: std::optional<AssetFileMetaData::ImporterInfo>{});
		return saveInfo;
	}

	template <typename T>
	bool AssetEditorSystem<T>::IsSavedToFile() const
	{
		const MementoAction* const topAction = mMementoStack.PeekTop();

		if (topAction == nullptr)
		{
			return true;
		}

		if (!std::filesystem::exists(mPathToSaveAssetTo))
		{
			return false;
		}

		const std::filesystem::file_time_type lastWriteTime = std::filesystem::last_write_time(mPathToSaveAssetTo);

		if (topAction->mTimeWeCheckedIfIsSameAsFile == lastWriteTime)
		{
			return topAction->mIsSameAsFile;
		}

		topAction->mTimeWeCheckedIfIsSameAsFile = lastWriteTime;

		if (mAssetOnFile.mWriteTimeAtTimeOfReserializing != lastWriteTime)
		{
			// Otherwise we load and save the file.
			LOG(LogEditor, Verbose, "Loading asset {} from file to check if it's unsaved...", mAsset.GetName());

			std::optional<AssetLoadInfo> loadInfo = AssetLoadInfo::LoadFromFile(mPathToSaveAssetTo);

			if (!loadInfo.has_value())
			{
				LOG(LogEditor, Error, "Could not load asset from file {}", mPathToSaveAssetTo.string());
				return false;
			}

			T assetAsSeenOnFile{ *loadInfo };
			mAssetOnFile.mReserializedAsset = Reserialize(assetAsSeenOnFile.Save().ToString());
			mAssetOnFile.mWriteTimeAtTimeOfReserializing = lastWriteTime;
		}

		topAction->mIsSameAsFile = mAssetOnFile.mReserializedAsset == topAction->mState;

		return topAction->mIsSameAsFile;
	}

	template <typename T>
	void AssetEditorSystem<T>::Tick(float deltaTime)
	{
		EditorSystem::Tick(deltaTime);

		if (Input::Get().IsKeyboardKeyHeld(Input::KeyboardKey::LeftControl)
			|| Input::Get().IsKeyboardKeyHeld(Input::KeyboardKey::RightControl))
		{
			if (mMementoStack.GetNumOfActionsDone() > 1 
				&& mMementoStack.CanUndo()
				&& Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::Z))
			{
				mMementoStack.Undo();
				mTimeLeftUntilCheckForDifference = sDifferenceCheckCoolDown;
				return;
			}

			if (mMementoStack.CanRedo()
				&& Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::Y))
			{
				mMementoStack.Redo();
				mTimeLeftUntilCheckForDifference = sDifferenceCheckCoolDown;
				return;
			}

			if (Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::S))
			{
				SaveToFile();
			}
		}

		mTimeLeftUntilCheckForDifference -= deltaTime;
		if (mTimeLeftUntilCheckForDifference <= 0.0f)
		{
			CheckForDifferences();
		}
	}

	template <typename T>
	bool AssetEditorSystem<T>::Begin(ImGuiWindowFlags flags)
	{
		return EditorSystem::Begin(flags | (IsSavedToFile() ? 0 : ImGuiWindowFlags_UnsavedDocument));
	}

	template<typename T>
	void AssetEditorSystem<T>::ShowSaveButton()
	{
		if (ImGui::Button(ICON_FA_FLOPPY_O))
		{
			// If you're looking at this because
			// you want to run logic before saving, look at ApplyChangesToAsset
			SaveToFile();
		}
	}

	template <typename T>
	void AssetEditorSystem<T>::CheckForDifferences()
	{
		mTimeLeftUntilCheckForDifference = sDifferenceCheckCoolDown;

		std::string currentStateAsString = Reserialize(SaveToMemory().ToString());
		MementoAction* const top = mMementoStack.PeekTop();

		if (top == nullptr)
		{
			LOG(LogEditor, Verbose, "No top action for {}, saving current state to memory", GetName());
			mMementoStack.Do(std::move(currentStateAsString), GetName());
			return;
		}

		if (top->mState == currentStateAsString)
		{
			return;
		}

		top->mState = Reserialize(top->mState);

		if (top->mState != currentStateAsString)
		{
			LOG(LogEditor, Verbose, "Change detected for {}", GetName());
			mMementoStack.Do(std::move(currentStateAsString), GetName());
		}
	}

	template <typename T>
	AssetEditorSystemInterface::MementoStack&& AssetEditorSystem<T>::ExtractMementoStack()
	{
		// It's possible an action was commited in the last .5f seconds, the change
		// has not been registered by the do-undo stack and would be ignored.
		if (mTimeLeftUntilCheckForDifference != sDifferenceCheckCoolDown)
		{
			CheckForDifferences();
		}

		return std::move(mMementoStack);
	}

	template <typename T>
	std::string AssetEditorSystem<T>::Reserialize(std::string_view serialized)
	{
		std::optional<AssetLoadInfo> loadInfo = AssetLoadInfo::LoadFromStream(std::make_unique<view_istream>(serialized));

		if (!loadInfo.has_value())
		{
			LOG(LogEditor, Error, "Failed to load metadata, metadata was invalid somehow?");
			return {};
		}

		T deserialized{ *loadInfo };
		AssetSaveInfo reserialized = deserialized.Save();
		return reserialized.ToString();
	}

	inline std::string Internal::GetSystemNameBasedOnAssetName(const std::string_view assetName)
	{
		return std::string{ assetName };
	}

	inline std::string Internal::GetAssetNameBasedOnSystemName(const std::string_view systemName)
	{
		return std::string{ systemName };
	}
}

#endif // EDITOR