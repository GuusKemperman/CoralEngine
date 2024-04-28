#include "Precomp.h"
#include "Assets/Core/AssetThumbnails.h"

#include "Assets/Level.h"
#include "Assets/Material.h"
#include "Assets/StaticMesh.h"
#include "Assets/Texture.h"
#include "Components/TransformComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/AssetManager.h"
#include "Rendering/FrameBuffer.h"
#include "Rendering/Renderer.h"
#include "World/Registry.h"
#include "World/World.h"

static std::string GetThumbnailName(std::string assetName)
{
	return CE::Format("{}'s Thumbnail", assetName);
}

std::unique_ptr<CE::ThumbnailFactory> CE::GetThumbNail(WeakAssetHandle<> forAsset)
{
	if (forAsset == nullptr)
	{
		return nullptr;
	}

	const MetaType& assetType = forAsset.GetMetaData().GetClass();
	const MetaFunc* const func = assetType.TryGetFunc(Internal::sGetThumbnailFuncName);

	if (func == nullptr
		|| func->GetParameters().size() != 2)
	{
		LOG(LogEditor, Error, "Could not get thumbnail for asset {} of type {}, function did not exist or had unexpected parameters",
			forAsset.GetMetaData().GetName(),
			assetType.GetName());
		return nullptr;
	}

	std::unique_ptr<ThumbnailFactory> returnValue{};
	const FuncResult result = func->InvokeCheckedUnpacked(returnValue, forAsset);

	if (result.HasError())
	{
		LOG(LogEditor, Error, "Could not get thumbnail for asset {} of type {}, function returned error {}",
			forAsset.GetMetaData().GetName(),
			assetType.GetName(),
			result.Error());
	}
	return returnValue;
}

CE::AssetHandle<CE::Texture> CE::GetDefaultThumbnail()
{
	return AssetManager::Get().TryGetAsset<Texture>("T_DefaultIcon");
}

CE::ThumbnailFromWorldFactory::ThumbnailFromWorldFactory(std::unique_ptr<World>&& world, 
	std::string_view thumbnailName) :
	mWorld(std::move(world)),
	mFrameBuffer(std::make_unique<FrameBuffer>(sDesiredThumbNailSize)),
	mTextureName(thumbnailName)
{
}

CE::ThumbnailFromWorldFactory::~ThumbnailFromWorldFactory() = default;

void CE::ThumbnailFromWorldFactory::Tick()
{
	ThumbnailFactory::Tick();

	mWorld->Tick(1.0f / 60.0f);
	CE::Renderer::Get().RenderToFrameBuffer(*mWorld, *mFrameBuffer, sDesiredThumbNailSize);

	if (++mNumOfTicksReceived == 2)
	{
		Texture texture{ mTextureName, std::move(*mFrameBuffer) };
		mTexture = AssetManager::Get().AddAsset(std::move(texture));
		mFrameBuffer.reset();
		mWorld.reset();
	}
}

template <>
std::unique_ptr<CE::ThumbnailFactory> GetThumbNailImpl<CE::Texture>(const CE::WeakAssetHandle<CE::Texture>& forAsset)
{
	return std::make_unique<CE::ThumbnailFactory>(CE::AssetHandle<CE::Texture>{ forAsset });
}

template <>
std::unique_ptr<CE::ThumbnailFactory> GetThumbNailImpl<CE::Script>(const CE::WeakAssetHandle<CE::Script>&)
{
	return std::make_unique<CE::ThumbnailFactory>(CE::AssetManager::Get().TryGetAsset<CE::Texture>("T_ScriptIcon"));
}

template <>
std::unique_ptr<CE::ThumbnailFactory> GetThumbNailImpl<CE::Level>(const CE::WeakAssetHandle<CE::Level>& forAsset)
{
	CE::World world = CE::AssetHandle<CE::Level>{ forAsset }->CreateWorld(false);
	return std::make_unique<CE::ThumbnailFromWorldFactory>(
		std::make_unique<CE::World>(std::move(world)), 
		GetThumbnailName(forAsset.GetMetaData().GetName()));
}

template <>
std::unique_ptr<CE::ThumbnailFactory> GetThumbNailImpl<CE::Prefab>(const CE::WeakAssetHandle<CE::Prefab>& forAsset)
{
	CE::World world = CE::Level::CreateDefaultWorld();
	world.GetRegistry().CreateFromPrefab(*CE::AssetHandle<CE::Prefab>{ forAsset });
	return std::make_unique<CE::ThumbnailFromWorldFactory>(
		std::make_unique<CE::World>(std::move(world)),
		GetThumbnailName(forAsset.GetMetaData().GetName()));
}

template <>
std::unique_ptr<CE::ThumbnailFactory> GetThumbNailImpl<CE::StaticMesh>(
	const CE::WeakAssetHandle<CE::StaticMesh>& forAsset)
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

	return std::make_unique<CE::ThumbnailFromWorldFactory>(
		std::make_unique<CE::World>(std::move(world)),
		GetThumbnailName(forAsset.GetMetaData().GetName()));
}
