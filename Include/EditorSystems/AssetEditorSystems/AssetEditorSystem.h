#include "Utilities/Time.h"
#ifdef EDITOR
#pragma once
#include "EditorSystems/EditorSystem.h"

#include "Assets/Asset.h"
#include "Assets/Core/AssetSaveInfo.h"
#include "Assets/Core/AssetLoadInfo.h"
#include "Core/AssetManager.h"
#include "Core/Editor.h"
#include "Core/Input.h"
#include "Meta/MetaType.h"
#include "Utilities/view_istream.h"
#include "Utilities/DoUndo.h"
#include "Utilities/StringFunctions.h"

namespace CE
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
			void Do();
			void Undo();
			void RefreshTheAssetEditor();

			std::string mState{};
			bool mDoIsNeeded{};

			// If the engine was refreshed,
			// we should re-serialize this
			// state in order to take into
			// account any changes made to
			// any assets.
			bool mRequiresReserialization{};

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
		WeakAssetHandle<T> TryGetOriginalAsset() const;

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

		void CompleteDifferenceCheckCycle();

		MementoStack&& ExtractMementoStack() override;

		/*
		 * Some assets are different every time we load them. entt::registry for example may completely
		 * shuffle all the entities around as it wishes. So we load and save the asset to account for this.
		 */
		static std::string Reserialize(std::string_view serialized);

		
		/**
		 * \brief Checking for differences involves saving/loading the asset a few times. We spread this out
		 * over several frames to reduce the impact of the frame drops.
		 */
		struct DifferenceCheckState
		{
			enum class Stage
			{
				SaveToMemory,
				ReloadFromMemory,
				ResaveToMemory,
				Compare,
				CheckIfSavedToFile,
				NUM_OF_STAGES,
				FirstStage = SaveToMemory
			};
			MementoAction mAction{};
			Stage mStage{};
			std::optional<T> mTemporarilyDeserializedAsset{};
		};
		DifferenceCheckState mDifferenceCheckState{};
		Cooldown mUpdateStateCooldown{ .4f };

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
				WeakAssetHandle<T> originalAsset = TryGetOriginalAsset();

				if (originalAsset != nullptr)
				{
					return originalAsset.GetFileOfOrigin().value_or(std::filesystem::path{});
				}
				return {};
			}
			())
	{
	}

	template<typename T>
	WeakAssetHandle<T> AssetEditorSystem<T>::TryGetOriginalAsset() const
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

		std::optional<AssetFileMetaData::ImporterInfo> importerInfo{};

		if (const WeakAssetHandle<T> originalAsset = TryGetOriginalAsset(); originalAsset != nullptr)
		{
			importerInfo = originalAsset.GetMetaData().GetImporterInfo();

			if (importerInfo.has_value())
			{
				importerInfo->mWereEditsMadeAfterImporting |= !IsSavedToFile();
			}
		}

		AssetSaveInfo saveInfo = mAsset.Save(std::move(importerInfo));
		return saveInfo;
	}

	template <typename T>
	bool AssetEditorSystem<T>::IsSavedToFile() const
	{
		const MementoAction* const topAction = mMementoStack.PeekTop();
		return topAction == nullptr ? true : topAction->mIsSameAsFile;
	}

	template <typename T>
	void AssetEditorSystem<T>::Tick(float deltaTime)
	{
		EditorSystem::Tick(deltaTime);

		const bool checkForDifferences = mUpdateStateCooldown.IsReady(deltaTime);

		if (!Input::Get().HasFocus())
		{
			return;
		}

		if (Input::Get().IsKeyboardKeyHeld(Input::KeyboardKey::LeftControl)
			|| Input::Get().IsKeyboardKeyHeld(Input::KeyboardKey::RightControl))
		{
			if (mMementoStack.GetNumOfActionsDone() > 1
				&& mMementoStack.CanUndo()
				&& Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::Z))
			{
				mMementoStack.Undo();
				mUpdateStateCooldown.mAmountOfTimePassed = 0.0f;
				mDifferenceCheckState.mStage = DifferenceCheckState::Stage::FirstStage;
				return;
			}

			if (mMementoStack.CanRedo()
				&& Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::Y))
			{
				mMementoStack.Redo();
				mUpdateStateCooldown.mAmountOfTimePassed = 0.0f;
				mDifferenceCheckState.mStage = DifferenceCheckState::Stage::FirstStage;
				return;
			}

			if (Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::S))
			{
				SaveToFile();
			}
		}

		if (checkForDifferences)
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
		mUpdateStateCooldown.mAmountOfTimePassed = 0.0f;

		switch (mDifferenceCheckState.mStage)
		{
		case DifferenceCheckState::Stage::SaveToMemory:
		{
			mDifferenceCheckState.mAction.mNameOfAssetEditor = GetName();
			mDifferenceCheckState.mAction.mState = SaveToMemory().ToString();

			if (mMementoStack.PeekTop() != nullptr
				&& mDifferenceCheckState.mAction.mState == mMementoStack.PeekTop()->mState)
			{
				mUpdateStateCooldown.mAmountOfTimePassed = -mUpdateStateCooldown.mCooldown * (static_cast<float>(DifferenceCheckState::Stage::NUM_OF_STAGES) - 1.0f);
				mDifferenceCheckState.mStage = DifferenceCheckState::Stage::CheckIfSavedToFile;
				return;
			}

			break;
		}
		case DifferenceCheckState::Stage::ReloadFromMemory:
		{
			std::optional<AssetLoadInfo> loadInfo = AssetLoadInfo::LoadFromStream(std::make_unique<view_istream>(mDifferenceCheckState.mAction.mState));

			if (!loadInfo.has_value())
			{
				LOG(LogEditor, Error, "Failed to load metadata, metadata was invalid somehow?");
				mDifferenceCheckState.mStage = DifferenceCheckState::Stage::CheckIfSavedToFile;
				return;
			}

			mDifferenceCheckState.mTemporarilyDeserializedAsset.emplace(*loadInfo);
			break;
		}
		case DifferenceCheckState::Stage::ResaveToMemory:
		{
			mDifferenceCheckState.mAction.mState = mDifferenceCheckState.mTemporarilyDeserializedAsset->Save().ToString();
			break;
		}
		case DifferenceCheckState::Stage::Compare:
		{
			MementoAction* const topAction = mMementoStack.PeekTop();

			if (topAction == nullptr)
			{
				// If this is the first action, it is likely
				// going to match the file exactly.
				// We check in more detail in the next stage.
				mDifferenceCheckState.mAction.mIsSameAsFile = true;

				mMementoStack.Do(std::move(mDifferenceCheckState.mAction));
				break;
			}

			if (topAction->mRequiresReserialization)
			{
				topAction->mState = Reserialize(topAction->mState);
				topAction->mRequiresReserialization = false;
			}

			if (topAction->mState != mDifferenceCheckState.mAction.mState)
			{
				LOG(LogEditor, Verbose, "Change detected for {}", GetName());

				// A change was made, it's unlikely going to match
				// the file. We check in more detail in the next stage.
				mDifferenceCheckState.mAction.mIsSameAsFile = false;

				mMementoStack.Do(std::move(mDifferenceCheckState.mAction));
			}

			break;
		}
		case DifferenceCheckState::Stage::CheckIfSavedToFile:
		{
			MementoAction* const topAction = mMementoStack.PeekTop();

			if (topAction == nullptr)
			{
				break;
			}

			if (!std::filesystem::exists(mPathToSaveAssetTo))
			{
				topAction->mIsSameAsFile = false;
				break;
			}

			const std::filesystem::file_time_type lastWriteTime = std::filesystem::last_write_time(mPathToSaveAssetTo);

			if (topAction->mTimeWeCheckedIfIsSameAsFile == lastWriteTime)
			{
				break;
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
					break;
				}

				T assetAsSeenOnFile{ *loadInfo };
				mAssetOnFile.mReserializedAsset = Reserialize(assetAsSeenOnFile.Save().ToString());
				mAssetOnFile.mWriteTimeAtTimeOfReserializing = lastWriteTime;
			}

			topAction->mIsSameAsFile = mAssetOnFile.mReserializedAsset == topAction->mState;

			break;
		}
		}

		mDifferenceCheckState.mStage = static_cast<typename DifferenceCheckState::Stage>(static_cast<int>(mDifferenceCheckState.mStage) + 1);

		if (mDifferenceCheckState.mStage == DifferenceCheckState::Stage::NUM_OF_STAGES)
		{
			mDifferenceCheckState.mStage = DifferenceCheckState::Stage::FirstStage;
		}
	}

	template <typename T>
	void AssetEditorSystem<T>::CompleteDifferenceCheckCycle()
	{
		for (int i = 0; i < static_cast<int>(DifferenceCheckState::Stage::NUM_OF_STAGES) * 2; i++)
		{
			CheckForDifferences();
		}
	}

	template <typename T>
	AssetEditorSystemInterface::MementoStack&& AssetEditorSystem<T>::ExtractMementoStack()
	{
		// It's possible an action was commited in the last .5f seconds, the change
		// has not been registered by the do-undo stack and would be ignored.
		if (mMementoStack.PeekTop() == nullptr
			|| (mUpdateStateCooldown.mAmountOfTimePassed != 0.0f || mDifferenceCheckState.mStage != DifferenceCheckState::Stage::FirstStage))
		{
			CompleteDifferenceCheckCycle();
		}

		for (std::unique_ptr<MementoAction>& action : mMementoStack.GetAllStoredActions())
		{
			action->mRequiresReserialization = true;
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