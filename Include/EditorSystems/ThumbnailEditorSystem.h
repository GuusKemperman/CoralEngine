#ifdef EDITOR
#include "EditorSystems/EditorSystem.h"

#include "World/World.h"
#include "Assets/Texture.h"
#include "Utilities/Time.h"
#include "Assets/Core/AssetHandle.h"
#include "Rendering/FrameBuffer.h"

namespace CE
{
	class Texture;
	class World;

	using GetThumbnailRet = std::variant<AssetHandle<Texture>, std::unique_ptr<World>>;

	class ThumbnailEditorSystem final :
		public EditorSystem
	{
	public:
		ThumbnailEditorSystem();

		void Tick(float deltaTime) override;

		ImTextureID GetThumbnail(const WeakAssetHandle<>& forAsset);

		bool DisplayImGuiImageButton(const WeakAssetHandle<>& forAsset, ImVec2 size);

		void DisplayImGuiImage(const WeakAssetHandle<>& forAsset, ImVec2 size);

		static constexpr glm::vec2 sGeneratedThumbnailResolution{ 80.0f };

	private:
		static ImTextureID GetDefaultThumbnail();

		static bool AreAllAssetsLoaded(const Timer& timeOut);

		template<typename T>
		static bool LoadAssets(const Timer& timeOut);

		// All thumbnails that are not used get offloaded after this number
		// multiplied by the amount of time it took to generate the thumbnail.
		// This keeps expensive thumbnails around for longer.
		static constexpr float sUnusedThumbnailRemoveStrictness = 2000.0f;

		static constexpr float sMinAmountOfTimeConsideredUnused = 2.0f;
		static constexpr float sMinAmountOfTimeConsideredUnusedWhenFullyRendered = 300.0f;

		Cooldown mRenderCooldown{ .25f };
		Cooldown mWorkCooldown{ 1.3f };
		static constexpr float sMaxTimeToSpendPerFrame = .04f;
		static constexpr uint32 sMaxNumOfFramesToRenderPerFrame = 10;

		struct GeneratedThumbnail
		{
			GeneratedThumbnail(FrameBuffer&& frameBuffer);
			GeneratedThumbnail(AssetHandle<Texture> texture);

			std::variant<AssetHandle<Texture>, Texture> mTexture{};
			Timer mNumSecondsSinceLastRequested{};
			float mTimeToGenerate{};
		};
		std::unordered_map<Name::HashType, GeneratedThumbnail> mGeneratedThumbnails{};

		struct GenerateRequest
		{
			WeakAssetHandle<> mForAsset{};
			Timer mNumSecondsSinceLastRequested{};
		};

		std::list<GenerateRequest> mGenerateQueue{};

		struct CurrentlyGenerating
		{
			std::unique_ptr<World> mWorld{};
			WeakAssetHandle<> mForAsset{};
			FrameBuffer mFrameBuffer{ sGeneratedThumbnailResolution };
			Timer mNumSecondsSinceLastRequested{};
			float mTimeNeededToCreateWorld{};
			bool mHasBeenRendered{};
		};
		std::list<CurrentlyGenerating> mCurrentlyGenerating{};

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
CE::GetThumbnailRet GetThumbNailImpl(const CE::WeakAssetHandle<CE::Level>& forAsset);

template<>
CE::GetThumbnailRet GetThumbNailImpl(const CE::WeakAssetHandle<CE::Prefab>& forAsset);

template<>
CE::GetThumbnailRet GetThumbNailImpl(const CE::WeakAssetHandle<CE::StaticMesh>& forAsset);

template<>
CE::GetThumbnailRet GetThumbNailImpl(const CE::WeakAssetHandle<CE::Material>& forAsset);

#endif // EDITOR