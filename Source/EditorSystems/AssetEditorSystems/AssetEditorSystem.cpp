#include "Precomp.h"
#include "EditorSystems/AssetEditorSystems/AssetEditorSystem.h"

#include "Assets/Core/AssetLoadInfo.h"
#include "Core/Input.h"
#include "Core/ThreadPool.h"

void CE::AssetEditorMementoStack::Do(std::shared_ptr<Action> action)
{
	ClearRedo();

	mActions.push_back(std::move(action));
	mNumOfActionsDone++;
}

bool CE::AssetEditorMementoStack::TryUndo()
{
	const bool canUndo = mNumOfActionsDone > 1;
	mNumOfActionsDone -= canUndo;
	return canUndo;
}

bool CE::AssetEditorMementoStack::TryRedo()
{
	const bool canRedo = mNumOfActionsDone < mActions.size();
	mNumOfActionsDone += canRedo;
	return canRedo;
}

void CE::AssetEditorMementoStack::ClearRedo()
{
	mActions.resize(mNumOfActionsDone);
}

std::shared_ptr<CE::AssetEditorMementoStack::Action> CE::AssetEditorMementoStack::GetMostRecentState() const
{
	if (mNumOfActionsDone == 0)
	{
		return nullptr;
	}
	ASSERT(mNumOfActionsDone <= mActions.size());

	return mActions[mNumOfActionsDone - 1];
}

CE::AssetEditorSystemBase::AssetEditorSystemBase(const Asset& asset) : // Don't hold onto this ref, it'll get invalidated
	EditorSystem(Internal::GetSystemNameBasedOnAssetName(asset.GetName()))
{
}

CE::AssetEditorSystemBase::~AssetEditorSystemBase()
{
	if (mActionToAdd.valid())
	{
		mActionToAdd.get();
	}
}

CE::WeakAssetHandle<CE::Asset> CE::AssetEditorSystemBase::TryGetOriginalAsset() const
{
	return AssetManager::Get().TryGetWeakAsset(GetAsset().GetName());
}

std::optional<std::filesystem::path> CE::AssetEditorSystemBase::GetDestinationFile() const
{
	const WeakAssetHandle original = TryGetOriginalAsset();

	if (original == nullptr)
	{
		return std::nullopt;
	}
	return original.GetFileOfOrigin();
}

void CE::AssetEditorSystemBase::SaveToFile()
{
	const std::optional<std::filesystem::path> dest = GetDestinationFile();

	if (!dest.has_value())
	{
		LOG(LogEditor, Error, "Could not save asset {} - no destination file path",
			GetAsset().GetName());
		return;
	}

	Editor::Get().Refresh(
		{
			Editor::RefreshRequest::Volatile,

			// A shared pointer, because it makes move semantics with lambdas so much simpler
			[assetSaveInfo = std::make_shared<AssetSaveInfo>(SaveToMemory()), saveTo = *dest]
			{
				assetSaveInfo->SaveToFile(saveTo);
			}
		});
}

CE::AssetSaveInfo CE::AssetEditorSystemBase::SaveToMemory()
{
	ApplyChangesToAsset();

	std::optional<AssetMetaData::ImporterInfo> importerInfo{};

	if (const WeakAssetHandle originalAsset = TryGetOriginalAsset(); originalAsset != nullptr)
	{
		importerInfo = originalAsset.GetMetaData().GetImporterInfo();

		if (importerInfo.has_value())
		{
			importerInfo->mWereEditsMadeAfterImporting |= !IsSavedToFile();
		}
	}

	AssetSaveInfo saveInfo = GetAsset().Save(std::move(importerInfo));
	return saveInfo;
}

bool CE::AssetEditorSystemBase::IsSavedToFile() const
{
	const std::shared_ptr<AssetEditorMementoStack::Action> mostRecentState = mMementoStack.GetMostRecentState();

	if (mostRecentState == nullptr)
	{
		return true;
	}
	return mostRecentState->mSimilarityToFile.mDoesStateMatchFile;
}

void CE::AssetEditorSystemBase::Tick(float deltaTime)
{
	EditorSystem::Tick(deltaTime);

	const bool checkForDifferences = mDifferenceCheckCooldown.IsReady(deltaTime);

	if (!Input::Get().HasFocus())
	{
		return;
	}

	const auto requestApplyStateChange = [this]
		{
			// The refreshing will return the asset editor to the
			// top state in the stack
			Editor::Get().Refresh({ 0, {}, GetName() });
			mDifferenceCheckCooldown.mAmountOfTimePassed = 0.0f;

			if (mActionToAdd.valid())
			{
				mActionToAdd.get();
			}
		};

	if (Input::Get().IsKeyboardKeyHeld(Input::KeyboardKey::LeftControl)
		|| Input::Get().IsKeyboardKeyHeld(Input::KeyboardKey::RightControl))
	{
		if (Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::Z)
			&& mMementoStack.TryUndo())
		{
			requestApplyStateChange();
			return;
		}

		if (Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::Y)
			&& mMementoStack.TryRedo())
		{
			requestApplyStateChange();
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

CE::AssetEditorMementoStack CE::AssetEditorSystemBase::ExtractStack()
{
	if (mActionToAdd.valid())
	{
		mActionToAdd.get();
		CheckForDifferences();
	}

	return std::move(mMementoStack);
}

void CE::AssetEditorSystemBase::InsertMementoStack(AssetEditorMementoStack stack)
{
	mMementoStack = std::move(stack);

	for (const std::shared_ptr<AssetEditorMementoStack::Action>& action : mMementoStack.mActions)
	{
		// Our asset might have depended on other assets
		// that have now been renamed, deleted, or otherwise
		// altered.
		action->mRequiresReserialization = true;
	}
}

bool CE::AssetEditorSystemBase::Begin(ImGuiWindowFlags flags)
{
	return EditorSystem::Begin(flags | (IsSavedToFile() ? 0 : ImGuiWindowFlags_UnsavedDocument));
}

void CE::AssetEditorSystemBase::ShowSaveButton()
{
	if (ImGui::Button(ICON_FA_FLOPPY_O))
	{
		SaveToFile();
	}
}

CE::MetaType CE::AssetEditorSystemBase::Reflect()
{
	return { MetaType::T<AssetEditorSystemBase>{}, "AssetEditorSystemBase", MetaType::Base<EditorSystem>{} };
}

void CE::AssetEditorSystemBase::CheckForDifferences()
{
	if (mActionToAdd.valid())
	{
		if (!IsFutureReady(mActionToAdd))
		{
			return;
		}

		std::optional<AssetEditorMementoStack::Action> change = mActionToAdd.get();

		if (change.has_value())
		{
			mMementoStack.Do(std::make_shared<AssetEditorMementoStack::Action>(std::move(*change)));
		}
	}

	const auto updateIsSameAsFile = [this](AssetEditorMementoStack::Action& action)
		{
			const std::optional<std::filesystem::path> destinationPath = GetDestinationFile();

			if (!destinationPath.has_value()
				|| !std::filesystem::exists(*destinationPath))
			{
				action.mSimilarityToFile = {};
				return;
			}

			const std::filesystem::file_time_type destinationWriteTime = std::filesystem::last_write_time(*destinationPath);

			if (mCachedFile.mLastWriteTime != destinationWriteTime)
			{
				mCachedFile.mLastWriteTime = destinationWriteTime;

				std::optional<AssetLoadInfo> loadInfo = AssetLoadInfo::LoadFromFile(*destinationPath);

				if (!loadInfo.has_value())
				{
					LOG(LogEditor, Error, "Could not load asset from file {}", destinationPath->string());
					action.mSimilarityToFile = {};
					return;
				}

				const auto assetPtr = ConstructAsset(*loadInfo);
				mCachedFile.mFileContents = Reserialize(assetPtr->Save().ToString());
			}

			if (action.mSimilarityToFile.mFileWriteTimeLastTimeWeChecked == mCachedFile.mLastWriteTime)
			{
				return;
			}

			if (action.mRequiresReserialization)
			{
				action.mState = Reserialize(action.mState);
				action.mRequiresReserialization = false;
			}

			action.mSimilarityToFile.mFileWriteTimeLastTimeWeChecked = mCachedFile.mLastWriteTime;
			action.mSimilarityToFile.mDoesStateMatchFile = action.mState == mCachedFile.mFileContents;
		};

	mActionToAdd = ThreadPool::Get().Enqueue(
		[this, 
			topAction = mMementoStack.GetMostRecentState(),
			currentState = SaveToMemory().ToString(),
			updateIsSameAsFile]() -> std::optional<AssetEditorMementoStack::Action>
		{
			if (topAction != nullptr)
			{
				updateIsSameAsFile(*topAction);
			}

			AssetEditorMementoStack::Action action{};
			action.mState = std::move(currentState);

			// Exactlyy the same as before, no changes were made
			if (topAction != nullptr
				&& action.mState == topAction->mState)
			{
				return std::nullopt;
			}

			action.mState = Reserialize(action.mState);

			if (topAction == nullptr)
			{
				updateIsSameAsFile(action);
				return action;
			}

			if (topAction->mRequiresReserialization)
			{
				topAction->mState = Reserialize(topAction->mState);
				topAction->mRequiresReserialization = false;
			}

			if (topAction->mState != action.mState)
			{
				LOG(LogEditor, Verbose, "Change detected for {}", GetName());
				updateIsSameAsFile(action);
				return action;
			}
			return std::nullopt;
		});
}

std::string CE::AssetEditorSystemBase::Reserialize(std::string_view serialized)
{
	std::optional<AssetLoadInfo> loadInfo = AssetLoadInfo::LoadFromStream(std::make_unique<view_istream>(serialized));

	if (!loadInfo.has_value())
	{
		LOG(LogEditor, Error, "Failed to load metadata, metadata was invalid somehow?");
		return {};
	}

	const auto assetPtr = ConstructAsset(*loadInfo);
	const AssetSaveInfo reserialized = assetPtr->Save();
	return reserialized.ToString();
}

std::string CE::Internal::GetSystemNameBasedOnAssetName(const std::string_view assetName)
{
	return std::string{ assetName };
}

std::string CE::Internal::GetAssetNameBasedOnSystemName(const std::string_view systemName)
{
	return std::string{ systemName };
}