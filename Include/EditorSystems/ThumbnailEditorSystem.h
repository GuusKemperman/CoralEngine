#ifdef EDITOR
#include "EditorSystems/EditorSystem.h"

#include "World/World.h"
#include "Assets/Texture.h"
#include "Utilities/Time.h"
#include "Assets/Core/AssetHandle.h"

namespace CE
{
	class Texture;
	class World;

	class ThumbnailEditorSystem final :
		public EditorSystem
	{
	public:
		ThumbnailEditorSystem();

		void Tick(float deltaTime) override;

		ImTextureID GetThumbnail(const WeakAssetHandle<>& forAsset);

		bool DisplayImGuiImageButton(const WeakAssetHandle<>& forAsset, ImVec2 size);

		void DisplayImGuiImage(const WeakAssetHandle<>& forAsset, ImVec2 size);

		static constexpr glm::vec2 sGeneratedThumbnailResolution{ 64.0f };

	private:
		static ImTextureID GetDefaultThumbnail();

		struct GeneratedThumbnail
		{
			GeneratedThumbnail(FrameBuffer&& frameBuffer);
			GeneratedThumbnail(AssetHandle<Texture> texture);

			std::variant<AssetHandle<Texture>, Texture> mTexture{};
			uint32 mNumOfTimesRequested = 1;
		};
		std::unordered_map<Name::HashType, GeneratedThumbnail> mGeneratedThumbnails{};

		struct GenerateRequest
		{
			WeakAssetHandle<> mForAsset{};
			uint32 mNumOfTimesRequested = 1;
		};

		std::list<GenerateRequest> mGenerateQueue{};

		struct CurrentlyGenerating
		{
			CurrentlyGenerating(World&& world, WeakAssetHandle<> forAsset);

			World mWorld;
			WeakAssetHandle<> mForAsset{};
			FrameBuffer mFrameBuffer{ sGeneratedThumbnailResolution };
			uint32 mNumOfTimesRendered{};
			uint32 mNumOfTimesRequested = 1;
		};
		std::list<CurrentlyGenerating> mCurrentlyGenerating{};

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ThumbnailEditorSystem);
	};
}

namespace CE
{
	class Ability;
	class SkinnedMesh;
	class Animation;
	class Material;
	class StaticMesh;
	class Level;
	class Prefab;
	class Texture;
	class Script;
	class FrameBuffer;

	using GetThumbnailRet = std::variant<AssetHandle<Texture>, World>;

	namespace Internal
	{
		GetThumbnailRet GenerateThumbnail(WeakAssetHandle<> forAsset);
		static constexpr std::string_view sGetThumbnailFuncName = "GenerateThumbnail";
	}
}

template<typename T>
CE::GetThumbnailRet GetThumbNailImpl([[maybe_unused]] const CE::WeakAssetHandle<T>& forAsset)
{
	return CE::AssetHandle<CE::Texture>{ };
}

template<>
CE::GetThumbnailRet GetThumbNailImpl(const CE::WeakAssetHandle<CE::Texture>& forAsset);

template<>
CE::GetThumbnailRet GetThumbNailImpl(const CE::WeakAssetHandle<CE::Script>& forAsset);

template<>
CE::GetThumbnailRet GetThumbNailImpl(const CE::WeakAssetHandle<CE::Animation>& forAsset);

template<>
CE::GetThumbnailRet GetThumbNailImpl(const CE::WeakAssetHandle<CE::Ability>& forAsset);

template<>
CE::GetThumbnailRet GetThumbNailImpl(const CE::WeakAssetHandle<CE::Level>& forAsset);

template<>
CE::GetThumbnailRet GetThumbNailImpl(const CE::WeakAssetHandle<CE::Prefab>& forAsset);

template<>
CE::GetThumbnailRet GetThumbNailImpl(const CE::WeakAssetHandle<CE::StaticMesh>& forAsset);

template<>
CE::GetThumbnailRet GetThumbNailImpl(const CE::WeakAssetHandle<CE::SkinnedMesh>& forAsset);

template<>
CE::GetThumbnailRet GetThumbNailImpl(const CE::WeakAssetHandle<CE::Material>& forAsset);

#endif // EDITOR