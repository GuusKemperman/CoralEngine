#include "Precomp.h"
#include "EditorSystems/ThumbnailEditorSystem.h"

#include "Core/AssetManager.h"
#include "Core/FileIO.h"
#include "Assets/Ability.h"
#include "Assets/Level.h"
#include "Assets/Material.h"
#include "Assets/StaticMesh.h"
#include "Assets/Texture.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/TransformComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/AssetManager.h"
#include "Rendering/FrameBuffer.h"
#include "Rendering/Renderer.h"
#include "World/Registry.h"
#include "World/World.h"

CE::ThumbnailEditorSystem::ThumbnailEditorSystem() :
	EditorSystem("ThumbnailEditorSystem")
{
}

void CE::ThumbnailEditorSystem::Tick(float deltaTime)
{
	EditorSystem::Tick(deltaTime);

	for (auto it = mGeneratedThumbnails.begin(); it != mGeneratedThumbnails.end();)
	{
		if (it->second.mNumOfTimesRequested == 0)
		{
			it = mGeneratedThumbnails.erase(it);
		}
		else
		{
			it->second.mNumOfTimesRequested = 0;
			++it;
		}
	}

	for (auto it = mCurrentlyGenerating.begin(); it != mCurrentlyGenerating.end();)
	{
		if (it->mNumOfTimesRequested == 0)
		{
			it = mCurrentlyGenerating.erase(it);
		}
		else
		{
			it->mNumOfTimesRequested = 0;
			++it;
		}
	}

	for (auto it = mGenerateQueue.begin(); it != mGenerateQueue.end();)
	{
		if (it->mNumOfTimesRequested == 0)
		{
			it = mGenerateQueue.erase(it);
			continue;
		}

		if (!it->mResult.IsReady())
		{
			it->mNumOfTimesRequested = 0;
			++it;
			continue;
		}

		GetThumbnailRet& generatedThumbnail = it->mResult.Get();

		if (std::holds_alternative<AssetHandle<Texture>>(generatedThumbnail))
		{
			AssetHandle<Texture> texture = std::move(std::get<AssetHandle<Texture>>(generatedThumbnail));

			if (texture != nullptr)
			{
				[[maybe_unused]] const auto result = mGeneratedThumbnails.emplace(std::piecewise_construct,
					std::forward_as_tuple(Name::HashString(it->mForAsset.GetMetaData().GetName())),
					std::forward_as_tuple(texture));

				if (!result.second)
				{
					LOG(LogEditor, Warning, "Could not add thumbnail to map.");
				}
			}
		}
		else
		{
			mCurrentlyGenerating.emplace_back(std::move(std::get<World>(generatedThumbnail)), it->mForAsset);
		}

		it = mGenerateQueue.erase(it);
	}

	if (mRenderFrameCooldown.IsReady(deltaTime)
		&& !mCurrentlyGenerating.empty())
	{
		const Timer timer{};

		bool anyNotSendYet{};

		for (WeakAssetHandle<Material> weakAsset : AssetManager::Get().GetAllAssets<Material>())
		{
			if (weakAsset.GetNumberOfStrongReferences() == 0)
			{
				continue;
			}

			AssetHandle<Material> material{ std::move(weakAsset) };

			// Load it in
			(void)*material;
		}

		while (!anyNotSendYet
			&& timer.GetSecondsElapsed() < .05f)
		{
			for (WeakAssetHandle<Texture> weakAsset : AssetManager::Get().GetAllAssets<Texture>())
			{
				if (weakAsset.GetNumberOfStrongReferences() == 0)
				{
					continue;
				}

				AssetHandle<Texture> texture{ std::move(weakAsset) };

				// Kickstarts the loading process
				(void)*texture;

				if (!texture->WasSendToGPU())
				{
					anyNotSendYet = true;
				}
			}

			if (!anyNotSendYet)
			{
				CurrentlyGenerating& current = mCurrentlyGenerating.front();
				Renderer::Get().RenderToFrameBuffer(current.mWorld, current.mFrameBuffer, current.mFrameBuffer.GetSize());

				[[maybe_unused]] const auto result = mGeneratedThumbnails.emplace(std::piecewise_construct,
					std::forward_as_tuple(Name::HashString(current.mForAsset.GetMetaData().GetName())),
					std::forward_as_tuple(std::move(current.mFrameBuffer)));

				if (!result.second)
				{
					LOG(LogEditor, Warning, "Could not add generated thumbnail to map.");
				}
				mCurrentlyGenerating.pop_front();
			}
		}
		mRenderFrameCooldown.mAmountOfTimePassed -= timer.GetSecondsElapsed();
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
		inGeneratedThumbnails->second.mNumOfTimesRequested++;

		const std::variant<AssetHandle<Texture>, Texture>& texture = inGeneratedThumbnails->second.mTexture;

		if (texture.index() == 1)
		{
			return std::get<1>(texture).GetImGuiId();
		}

		const AssetHandle<Texture> handle = std::get<0>(texture);
		return handle == nullptr ? nullptr : handle->GetImGuiId();
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
		inQueue->mNumOfTimesRequested++;
	}

	if (inCurrentlyGenerating != mCurrentlyGenerating.end())
	{
		inCurrentlyGenerating->mNumOfTimesRequested++;
	}

	if (inQueue == mGenerateQueue.end()
		&& inCurrentlyGenerating == mCurrentlyGenerating.end())
	{
		mGenerateQueue.emplace_back(forAsset);
	}

	return GetDefaultThumbnail();
}

bool CE::ThumbnailEditorSystem::DisplayImGuiImageButton(const WeakAssetHandle<>& forAsset, ImVec2 size)
{
	const ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
	return ImGui::ImageButton(ImGui::IsRectVisible(cursorScreenPos, cursorScreenPos + size) ? GetThumbnail(forAsset) : GetDefaultThumbnail(), size);
}

void CE::ThumbnailEditorSystem::DisplayImGuiImage(const WeakAssetHandle<>& forAsset, ImVec2 size)
{
	const ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
	ImGui::Image(ImGui::IsRectVisible(cursorScreenPos, cursorScreenPos + size) ? GetThumbnail(forAsset) : GetDefaultThumbnail(), size);
}

ImTextureID CE::ThumbnailEditorSystem::GetDefaultThumbnail()
{
	AssetHandle<Texture> icon = AssetManager::Get().TryGetAsset<Texture>("T_DefaultIcon");
	return icon == nullptr ? nullptr : icon->GetImGuiId();
}

CE::ThumbnailEditorSystem::GeneratedThumbnail::GeneratedThumbnail(FrameBuffer&& frameBuffer)
{
	mTexture.emplace<Texture>(std::string_view{}, std::move(frameBuffer));
}

CE::ThumbnailEditorSystem::GeneratedThumbnail::GeneratedThumbnail(AssetHandle<Texture> texture) :
	mTexture(texture)
{
}

CE::ThumbnailEditorSystem::GenerateRequest::GenerateRequest(WeakAssetHandle<> handle) :
	mForAsset(handle),
	mResult([handle]{ return Internal::GenerateThumbnail(handle); })
{
}

CE::ThumbnailEditorSystem::GenerateRequest::~GenerateRequest()
{
	mResult.GetThread().CancelOrJoin();
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
CE::GetThumbnailRet GetThumbNailImpl<CE::Animation>(const CE::WeakAssetHandle<CE::Animation>&)
{
	return CE::AssetManager::Get().TryGetAsset<CE::Texture>("T_AnimationsIcon");
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
CE::GetThumbnailRet GetThumbNailImpl<CE::SkinnedMesh>(const CE::WeakAssetHandle<CE::SkinnedMesh>& forAsset)
{
	CE::World world = CE::Level::CreateDefaultWorld();

	{
		CE::Registry& reg = world.GetRegistry();
		const entt::entity entity = reg.Create();

		reg.AddComponent<CE::TransformComponent>(entity);
		CE::SkinnedMeshComponent& skinnedMeshComponent = reg.AddComponent<CE::SkinnedMeshComponent>(entity);

		skinnedMeshComponent.mSkinnedMesh = CE::AssetHandle<CE::SkinnedMesh>{ forAsset };
		skinnedMeshComponent.mMaterial = CE::Material::TryGetDefaultMaterial();
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
