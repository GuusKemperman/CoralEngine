#include "Precomp.h"
#include "EditorSystems/ThumbnailEditorSystem.h"

#include "Core/AssetManager.h"
#include "Core/FileIO.h"
#include "Assets/Level.h"
#include "Assets/Material.h"
#include "Assets/StaticMesh.h"
#include "Assets/Texture.h"
#include "Components/TransformComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/Renderer.h"
#include "Core/ThreadPool.h"
#include "Rendering/FrameBuffer.h"
#include "World/Registry.h"
#include "World/World.h"

CE::ThumbnailEditorSystem::ThumbnailEditorSystem() :
	EditorSystem("ThumbnailEditorSystem")
{
}

CE::AssetHandle<CE::Texture> CE::ThumbnailEditorSystem::GetThumbnail(const WeakAssetHandle<>& forAsset)
{
	if (forAsset == nullptr)
	{
		return GetDefaultThumbnail();
	}

	const auto inGeneratedThumbnails = mThumbnails.find(Name::HashString(forAsset.GetMetaData().GetName()));

	if (inGeneratedThumbnails != mThumbnails.end())
	{
		if (IsFutureReady(inGeneratedThumbnails->second))
		{
			return inGeneratedThumbnails->second.get();
		}
		return GetDefaultThumbnail();
	}

	std::future thumbnailFuture = ThreadPool::Get().Enqueue(&Internal::GenerateThumbnail, forAsset);
	mThumbnails.emplace(Name::HashString(forAsset.GetMetaData().GetName()), thumbnailFuture.share());

	return GetDefaultThumbnail();
}

bool CE::ThumbnailEditorSystem::DisplayImGuiImageButton(const WeakAssetHandle<>& forAsset, ImVec2 size)
{
	const ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
	const std::string_view id = forAsset != nullptr ? std::string_view{ forAsset.GetMetaData().GetName() } : "None";

	const AssetHandle<Texture> thumbnail = ImGui::IsRectVisible(cursorScreenPos, cursorScreenPos + size) ? GetThumbnail(forAsset) : GetDefaultThumbnail();

	return ImGui::ImageButton(id.data(), 
		Renderer::Get().GetPlatformId(thumbnail), 
		size,
		ImVec2(0, 1),
		ImVec2(1, 0));
}

void CE::ThumbnailEditorSystem::DisplayImGuiImage(const WeakAssetHandle<>& forAsset, ImVec2 size)
{
	const ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
	const AssetHandle<Texture> thumbnail = ImGui::IsRectVisible(cursorScreenPos, cursorScreenPos + size) ? GetThumbnail(forAsset) : GetDefaultThumbnail();

	ImGui::Image(Renderer::Get().GetPlatformId(thumbnail),
		size,
		ImVec2(0, 1),
		ImVec2(1, 0));
}

CE::AssetHandle<CE::Texture> CE::ThumbnailEditorSystem::GetDefaultThumbnail()
{
	return AssetManager::Get().TryGetAsset<Texture>("T_DefaultIcon");
}

CE::AssetHandle<CE::Texture> CE::Internal::GenerateThumbnail(WeakAssetHandle<> forAsset)
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

	AssetHandle<Texture> returnValue{};
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

CE::AssetHandle<CE::Texture> CE::RenderWorldToThumbnail(World& world)
{
	FrameBuffer frameBuffer{ ThumbnailEditorSystem::sGeneratedThumbnailResolution };
	world.Render(glm::vec2{}, &frameBuffer);
	return MakeAssetHandle<Texture>("Thumbnail", std::move(frameBuffer));
}

template <>
CE::AssetHandle<CE::Texture> GetThumbNailImpl<CE::Texture>(const CE::WeakAssetHandle<CE::Texture>& forAsset)
{
	return forAsset;
}

template <>
CE::AssetHandle<CE::Texture> GetThumbNailImpl<CE::Script>(const CE::WeakAssetHandle<CE::Script>&)
{
	return CE::AssetManager::Get().TryGetAsset<CE::Texture>("T_ScriptIcon");
}

template <>
CE::AssetHandle<CE::Texture> GetThumbNailImpl<CE::Level>(const CE::WeakAssetHandle<CE::Level>& forAsset)
{
	const std::unique_ptr<CE::World> world = CE::AssetHandle{ forAsset }->CreateWorld(false);
	return CE::RenderWorldToThumbnail(*world);
}

template <>
CE::AssetHandle<CE::Texture> GetThumbNailImpl<CE::Prefab>(const CE::WeakAssetHandle<CE::Prefab>& forAsset)
{
	const std::unique_ptr<CE::World> world = CE::Level::CreateDefaultWorld();
	world->GetRegistry().CreateFromPrefab(*CE::AssetHandle{ forAsset });
	return CE::RenderWorldToThumbnail(*world);
}

template <>
CE::AssetHandle<CE::Texture> GetThumbNailImpl<CE::StaticMesh>(const CE::WeakAssetHandle<CE::StaticMesh>& forAsset)
{
	const std::unique_ptr<CE::World> world = CE::Level::CreateDefaultWorld();
	CE::Registry& reg = world->GetRegistry();

	{
		const entt::entity entity = reg.Create();

		reg.AddComponent<CE::TransformComponent>(entity);
		CE::StaticMeshComponent& staticMeshComponent = reg.AddComponent<CE::StaticMeshComponent>(entity);

		staticMeshComponent.mStaticMesh = CE::AssetHandle<CE::StaticMesh>{ forAsset };
		staticMeshComponent.mMaterial = CE::Material::TryGetDefaultMaterial();
	}

	return CE::RenderWorldToThumbnail(*world);
}

template <>
CE::AssetHandle<CE::Texture> GetThumbNailImpl<CE::Material>(const CE::WeakAssetHandle<CE::Material>& forAsset)
{
	const std::unique_ptr<CE::World> world = CE::Level::CreateDefaultWorld();
	CE::Registry& reg = world->GetRegistry();

	{
		const entt::entity entity = reg.Create();

		reg.AddComponent<CE::TransformComponent>(entity).SetLocalScale(2.5f);
		CE::StaticMeshComponent& staticMeshComponent = reg.AddComponent<CE::StaticMeshComponent>(entity);

		staticMeshComponent.mStaticMesh = CE::AssetManager::Get().TryGetAsset<CE::StaticMesh>("SM_Sphere");

		if (staticMeshComponent.mStaticMesh == nullptr)
		{
			LOG(LogEditor, Warning, "The default asset used for creating thumbnails for materials has been renamed or removed. Materials will no longer have a thumbnail");
			return CE::AssetHandle<CE::Texture>{ nullptr };
		}

		staticMeshComponent.mMaterial = CE::AssetHandle<CE::Material>{ forAsset };
	}

	return CE::RenderWorldToThumbnail(*world);
}
