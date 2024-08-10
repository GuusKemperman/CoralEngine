#include "Precomp.h"
#include "EditorSystems/ThumbnailEditorSystem.h"

#include "Core/AssetManager.h"
#include "Core/FileIO.h"
#include "Assets/Ability/Ability.h"
#include "Assets/Level.h"
#include "Assets/Material.h"
#include "Assets/StaticMesh.h"
#include "Assets/Texture.h"
#include "Assets/Ability/Weapon.h"
#include "Components/TransformComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/Renderer.h"
#include "Rendering/FrameBuffer.h"
#include "World/Registry.h"
#include "World/World.h"

CE::ThumbnailEditorSystem::ThumbnailEditorSystem() :
	EditorSystem("ThumbnailEditorSystem")
{
}

void CE::ThumbnailEditorSystem::Tick(float deltaTime)
{
	const Timer timeSpendThisFrame{};

	if (mRenderCooldown.IsReady(deltaTime)
		&& !mCurrentlyGenerating.empty() 
		&& AreAllAssetsLoaded(timeSpendThisFrame))
	{
		auto it = mCurrentlyGenerating.begin();
		for (uint32 numRendered = 0; it != mCurrentlyGenerating.end() 
			&& numRendered < sMaxNumOfFramesToRenderPerFrame 
			&& timeSpendThisFrame.GetSecondsElapsed() <= sMaxTimeToSpendPerFrame; ++it, ++numRendered)
		{
			CurrentlyGenerating& current = *it;
			Timer timeToRenderFrame{};

			LOG(LogEditor, Verbose, "Rendering {} to thumbnail", current.mForAsset.GetMetaData().GetName());
			
			current.mWorld.Render(&current.mFrameBuffer);

			const auto result = mGeneratedThumbnails.emplace(std::piecewise_construct,
				std::forward_as_tuple(Name::HashString(current.mForAsset.GetMetaData().GetName())),
				std::forward_as_tuple(std::move(current.mFrameBuffer)));

			result.first->second.mTimeToGenerate = timeToRenderFrame.GetSecondsElapsed() + current.mTimeNeededToCreateWorld;
		}
		mCurrentlyGenerating.erase(mCurrentlyGenerating.begin(), it);
	}

	if (!mWorkCooldown.IsReady(deltaTime))
	{
		return;
	}

	for (auto it = mGeneratedThumbnails.begin(); it != mGeneratedThumbnails.end() && timeSpendThisFrame.GetSecondsElapsed() <= sMaxTimeToSpendPerFrame;)
	{
		if (it->second.mNumSecondsSinceLastRequested.GetSecondsElapsed() >= sMinAmountOfTimeConsideredUnusedWhenFullyRendered
			&& it->second.mNumSecondsSinceLastRequested.GetSecondsElapsed() >= it->second.mTimeToGenerate * sUnusedThumbnailRemoveStrictness)
		{
			it = mGeneratedThumbnails.erase(it);
		}
		else
		{
			++it;
		}
	}

	for (auto it = mCurrentlyGenerating.begin(); it != mCurrentlyGenerating.end() && timeSpendThisFrame.GetSecondsElapsed() <= sMaxTimeToSpendPerFrame;)
	{
		if (it->mNumSecondsSinceLastRequested.GetSecondsElapsed() >= sMinAmountOfTimeConsideredUnused
			&& it->mNumSecondsSinceLastRequested.GetSecondsElapsed() >= it->mTimeNeededToCreateWorld * sUnusedThumbnailRemoveStrictness)
		{
			it = mCurrentlyGenerating.erase(it);
		}
		else
		{
			++it;
		}
	}

	std::stable_sort(mGenerateQueue.begin(), mGenerateQueue.end(),
		[](const GenerateRequest& lhs, const GenerateRequest& rhs)
		{
			return lhs.mNumSecondsSinceLastRequested.GetSecondsElapsed() > rhs.mNumSecondsSinceLastRequested.GetSecondsElapsed();
		});

	for (auto it = mGenerateQueue.begin(); it != mGenerateQueue.end() && timeSpendThisFrame.GetSecondsElapsed() <= sMaxTimeToSpendPerFrame;)
	{
		if (it->mNumSecondsSinceLastRequested.GetSecondsElapsed() >= sMinAmountOfTimeConsideredUnused)
		{
			it = mGenerateQueue.erase(it);
			continue;
		}

		const Timer timeToGenerate{};
		GetThumbnailRet generatedThumbnail = Internal::GenerateThumbnail(it->mForAsset);

		if (std::holds_alternative<AssetHandle<Texture>>(generatedThumbnail))
		{
			AssetHandle<Texture> texture = std::move(std::get<AssetHandle<Texture>>(generatedThumbnail));

			if (texture != nullptr)
			{
				const auto result = mGeneratedThumbnails.emplace(std::piecewise_construct,
					std::forward_as_tuple(Name::HashString(it->mForAsset.GetMetaData().GetName())),
					std::forward_as_tuple(texture));

				result.first->second.mTimeToGenerate = timeToGenerate.GetSecondsElapsed();
			}
		}
		else
		{
			CurrentlyGenerating& result = mCurrentlyGenerating.emplace_back(std::move(std::get<World>(generatedThumbnail)), it->mForAsset);
			result.mTimeNeededToCreateWorld = timeToGenerate.GetSecondsElapsed();
		}

		it = mGenerateQueue.erase(it);
	}

	if (!mCurrentlyGenerating.empty())
	{
		// Starts the loading process
		// for all textures
		(void)AreAllAssetsLoaded(timeSpendThisFrame);
	}
}

ImTextureID CE::ThumbnailEditorSystem::GetThumbnail(const WeakAssetHandle<>& forAsset)
{
	if (forAsset == nullptr)
	{
		return GetDefaultThumbnail();
	}

	const auto inGeneratedThumbnails = mGeneratedThumbnails.find(Name::HashString(forAsset.GetMetaData().GetName()));

	if (inGeneratedThumbnails != mGeneratedThumbnails.end())
	{
		inGeneratedThumbnails->second.mNumSecondsSinceLastRequested.Reset();

		const std::variant<AssetHandle<Texture>, Texture>& texture = inGeneratedThumbnails->second.mTexture;

		if (texture.index() == 1)
		{
			return Renderer::Get().GetPlatformId(std::get<1>(texture).GetPlatformImpl());
		}

		const AssetHandle<Texture> handle = std::get<0>(texture);
		return handle == nullptr ? nullptr : Renderer::Get().GetPlatformId(handle->GetPlatformImpl());
	}

	const auto inQueue = std::find_if(mGenerateQueue.begin(), mGenerateQueue.end(), 
		[&forAsset](const GenerateRequest& request)
		{
			return request.mForAsset == forAsset;
		});

	const auto inCurrentlyGenerating = std::find_if(mCurrentlyGenerating.begin(), mCurrentlyGenerating.end(), 
		[&forAsset](const CurrentlyGenerating& current)
		{
			return current.mForAsset == forAsset;
		});

	if (inQueue != mGenerateQueue.end())
	{
		inQueue->mNumSecondsSinceLastRequested.Reset();
	}

	if (inCurrentlyGenerating != mCurrentlyGenerating.end())
	{
		inCurrentlyGenerating->mNumSecondsSinceLastRequested.Reset();
	}

	if (inQueue == mGenerateQueue.end()
		&& inCurrentlyGenerating == mCurrentlyGenerating.end())
	{
		mGenerateQueue.emplace_back(GenerateRequest{ forAsset });
	}

	return GetDefaultThumbnail();
}

bool CE::ThumbnailEditorSystem::DisplayImGuiImageButton(const WeakAssetHandle<>& forAsset, ImVec2 size)
{
	const ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
	const std::string_view id = forAsset != nullptr ? std::string_view{ forAsset.GetMetaData().GetName() } : "None";
	return ImGui::ImageButton(id.data(), ImGui::IsRectVisible(cursorScreenPos, cursorScreenPos + size) ? GetThumbnail(forAsset) : GetDefaultThumbnail(), size);
}

void CE::ThumbnailEditorSystem::DisplayImGuiImage(const WeakAssetHandle<>& forAsset, ImVec2 size)
{
	const ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
	ImGui::Image(ImGui::IsRectVisible(cursorScreenPos, cursorScreenPos + size) ? GetThumbnail(forAsset) : GetDefaultThumbnail(), size);
}

ImTextureID CE::ThumbnailEditorSystem::GetDefaultThumbnail()
{
	AssetHandle<Texture> icon = AssetManager::Get().TryGetAsset<Texture>("T_DefaultIcon");
	return icon == nullptr ? nullptr : Renderer::Get().GetPlatformId(icon->GetPlatformImpl());
}

bool CE::ThumbnailEditorSystem::AreAllAssetsLoaded(const Timer& timeOut)
{
	bool allLoaded = LoadAssets<Material>(timeOut)
		&& LoadAssets<StaticMesh>(timeOut);

	for (WeakAssetHandle<Texture> weakAsset : AssetManager::Get().GetAllAssets<Texture>())
	{
		if (timeOut.GetSecondsElapsed() > sMaxTimeToSpendPerFrame)
		{
			return false;
		}

		if (weakAsset.GetNumberOfStrongReferences() == 0)
		{
			continue;
		}

		AssetHandle<Texture> texture{ std::move(weakAsset) };

		// Kickstarts the loading process
		(void)*texture;
	}

	return allLoaded;
}

template <typename T>
bool CE::ThumbnailEditorSystem::LoadAssets(const Timer& timeOut)
{
	for (WeakAssetHandle<T> weakAsset : AssetManager::Get().GetAllAssets<T>())
	{
		if (timeOut.GetSecondsElapsed() > sMaxTimeToSpendPerFrame)
		{
			return false;
		}

		if (weakAsset.GetNumberOfStrongReferences() == 0)
		{
			continue;
		}

		AssetHandle<T> asset{ std::move(weakAsset) };

		// Load it in
		(void)*asset;
	}

	return true;
}

CE::ThumbnailEditorSystem::GeneratedThumbnail::GeneratedThumbnail(FrameBuffer&& frameBuffer)
{
	mTexture.emplace<Texture>(std::string_view{}, std::move(frameBuffer));
}

CE::ThumbnailEditorSystem::GeneratedThumbnail::GeneratedThumbnail(AssetHandle<Texture> texture) :
	mTexture(texture)
{
}

CE::ThumbnailEditorSystem::CurrentlyGenerating::CurrentlyGenerating(World&& world, WeakAssetHandle<> forAsset) :
	mWorld(std::move(world)),
	mForAsset(std::move(forAsset))
{
}

CE::GetThumbnailRet CE::Internal::GenerateThumbnail(WeakAssetHandle<> forAsset)
{
	if (forAsset == nullptr)
	{
		return AssetHandle<Texture>{};
	}

	const MetaType& assetType = forAsset.GetMetaData().GetClass();
	const MetaFunc* const func = assetType.TryGetFunc(sGetThumbnailFuncName);

	if (func == nullptr
		|| func->GetParameters().size() != 2)
	{
		LOG(LogEditor, Error, "Could not get thumbnail for asset {} of type {}, function did not exist or had unexpected parameters",
			forAsset.GetMetaData().GetName(),
			assetType.GetName());
		return AssetHandle<Texture>{};
	}

	GetThumbnailRet returnValue{};
	const FuncResult result = func->InvokeUncheckedUnpacked(returnValue, forAsset);

	if (result.HasError())
	{
		LOG(LogEditor, Error, "Could not get thumbnail for asset {} of type {}, function returned error {}",
			forAsset.GetMetaData().GetName(),
			assetType.GetName(),
			result.Error());
	}
	return returnValue;
}

CE::MetaType CE::ThumbnailEditorSystem::Reflect()
{
	MetaType type{ MetaType::T<ThumbnailEditorSystem>{}, "ThumbnailEditorSystem", MetaType::Base<EditorSystem>{} };
	type.GetProperties().Add(Props::sEditorSystemAlwaysOpenTag);
	return type;
}

template <>
CE::GetThumbnailRet GetThumbNailImpl<CE::Texture>(const CE::WeakAssetHandle<CE::Texture>& forAsset)
{
	return CE::AssetHandle<CE::Texture>{ forAsset };
}

template <>
CE::GetThumbnailRet GetThumbNailImpl<CE::Script>(const CE::WeakAssetHandle<CE::Script>&)
{
	return CE::AssetManager::Get().TryGetAsset<CE::Texture>("T_ScriptIcon");
}

template <>
CE::GetThumbnailRet GetThumbNailImpl<CE::Ability>(const CE::WeakAssetHandle<CE::Ability>& forAsset)
{
	CE::AssetHandle icon = CE::AssetHandle<CE::Ability>{ forAsset }->GetIconTexture();

	if (icon == nullptr)
	{
		icon = CE::AssetManager::Get().TryGetAsset<CE::Texture>("T_AbilityIcon");
	}

	return icon;
}

template <>
CE::GetThumbnailRet GetThumbNailImpl<CE::Weapon>(const CE::WeakAssetHandle<CE::Weapon>& forAsset)
{
	CE::AssetHandle icon = CE::AssetHandle<CE::Weapon>{ forAsset }->GetIconTexture();

	if (icon == nullptr)
	{
		icon = CE::AssetManager::Get().TryGetAsset<CE::Texture>("T_WeaponIcon");
	}

	return icon;
}

template <>
CE::GetThumbnailRet GetThumbNailImpl<CE::Level>(const CE::WeakAssetHandle<CE::Level>& forAsset)
{
	return CE::AssetHandle<CE::Level>{ forAsset }->CreateWorld(false);
}

template <>
CE::GetThumbnailRet GetThumbNailImpl<CE::Prefab>(const CE::WeakAssetHandle<CE::Prefab>& forAsset)
{
	CE::World world = CE::Level::CreateDefaultWorld();
	world.GetRegistry().CreateFromPrefab(*CE::AssetHandle<CE::Prefab>{ forAsset });
	return world;
}

template <>
CE::GetThumbnailRet GetThumbNailImpl<CE::StaticMesh>(const CE::WeakAssetHandle<CE::StaticMesh>& forAsset)
{
	CE::World world = CE::Level::CreateDefaultWorld();

	{
		CE::Registry& reg = world.GetRegistry();
		const entt::entity entity = reg.Create();

		reg.AddComponent<CE::TransformComponent>(entity);
		CE::StaticMeshComponent& staticMeshComponent = reg.AddComponent<CE::StaticMeshComponent>(entity);

		staticMeshComponent.mStaticMesh = CE::AssetHandle<CE::StaticMesh>{ forAsset };
		staticMeshComponent.mMaterial = CE::Material::TryGetDefaultMaterial();
	}

	return world;
}

template <>
CE::GetThumbnailRet GetThumbNailImpl<CE::Material>(const CE::WeakAssetHandle<CE::Material>& forAsset)
{
	CE::World world = CE::Level::CreateDefaultWorld();

	{
		CE::Registry& reg = world.GetRegistry();
		const entt::entity entity = reg.Create();

		reg.AddComponent<CE::TransformComponent>(entity).SetLocalScale(2.5f);
		CE::StaticMeshComponent& staticMeshComponent = reg.AddComponent<CE::StaticMeshComponent>(entity);

		staticMeshComponent.mStaticMesh = CE::AssetManager::Get().TryGetAsset<CE::StaticMesh>("Sphere_Sphere");

		if (staticMeshComponent.mStaticMesh == nullptr)
		{
			LOG(LogEditor, Warning, "The default asset used for creating thumbnails for materials has been renamed or removed. Materials will no longer have a thumbnail");
			return CE::AssetHandle<CE::Texture>{ nullptr };
		}

		staticMeshComponent.mMaterial = CE::AssetHandle<CE::Material>{ forAsset };
	}

	return world;
}
