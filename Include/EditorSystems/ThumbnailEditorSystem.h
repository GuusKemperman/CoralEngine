#ifdef EDITOR
#include "EditorSystems/EditorSystem.h"

#include <future>

#include "Assets/Texture.h"
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

		AssetHandle<Texture> GetThumbnail(const WeakAssetHandle<>& forAsset);

		bool DisplayImGuiImageButton(const WeakAssetHandle<>& forAsset, ImVec2 size);

		void DisplayImGuiImage(const WeakAssetHandle<>& forAsset, ImVec2 size);

		static constexpr glm::vec2 sGeneratedThumbnailResolution{ 80.0f };

	private:
		static AssetHandle<Texture> GetDefaultThumbnail();

		std::unordered_map<Name::HashType, std::shared_future<AssetHandle<Texture>>> mThumbnails{};

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ThumbnailEditorSystem);
	};
}

namespace CE
{
	class Material;
	class StaticMesh;
	class Level;
	class Prefab;
	class Texture;
	class Script;
	class FrameBuffer;

	namespace Internal
	{
		AssetHandle<Texture> GenerateThumbnail(WeakAssetHandle<> forAsset);
		static constexpr std::string_view sGetThumbnailFuncName = "GenerateThumbnail";
	}

	AssetHandle<Texture> RenderWorldToThumbnail(World& world);
}

template<typename T>
CE::AssetHandle<CE::Texture> GetThumbNailImpl([[maybe_unused]] const CE::WeakAssetHandle<T>& forAsset)
{
	return CE::AssetHandle<CE::Texture>{ };
}

template<>
CE::AssetHandle<CE::Texture> GetThumbNailImpl(const CE::WeakAssetHandle<CE::Texture>& forAsset);

template<>
CE::AssetHandle<CE::Texture> GetThumbNailImpl(const CE::WeakAssetHandle<CE::Script>& forAsset);

template<>
CE::AssetHandle<CE::Texture> GetThumbNailImpl(const CE::WeakAssetHandle<CE::Level>& forAsset);

template<>
CE::AssetHandle<CE::Texture> GetThumbNailImpl(const CE::WeakAssetHandle<CE::Prefab>& forAsset);

template<>
CE::AssetHandle<CE::Texture> GetThumbNailImpl(const CE::WeakAssetHandle<CE::StaticMesh>& forAsset);

template<>
CE::AssetHandle<CE::Texture> GetThumbNailImpl(const CE::WeakAssetHandle<CE::Material>& forAsset);

#endif // EDITOR