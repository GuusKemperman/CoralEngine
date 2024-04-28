#pragma once
#ifdef EDITOR

#include "Assets/Core/AssetHandle.h"

namespace CE
{
	class SkinnedMesh;
	class Animation;
	class Material;
	class StaticMesh;
	class Level;
	class Prefab;
	class Texture;
	class Script;
	class World;
	class FrameBuffer;
	class ThumbnailFactory;

	static constexpr glm::vec2 sDesiredThumbNailSize{ 64.0f };

	std::unique_ptr<ThumbnailFactory> GetThumbNail(WeakAssetHandle<> forAsset);

	AssetHandle<Texture> GetDefaultThumbnail();

	namespace Internal
	{
		static constexpr std::string_view sGetThumbnailFuncName = "GetThumbNail";
	}

	class ThumbnailFactory
	{
	public:
		ThumbnailFactory() = default;
		ThumbnailFactory(const AssetHandle<Texture>& texture) :
			mTexture(texture)
		{}

		virtual ~ThumbnailFactory() = default;

		virtual void Tick() {};

		AssetHandle<Texture> mTexture{};

		enum class State
		{
			NeedMoreTicks,
			Failed,
		};
		State mState{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
	};

	class ThumbnailFromWorldFactory :
		public ThumbnailFactory
	{
	public:
		ThumbnailFromWorldFactory(std::unique_ptr<World>&& world, std::string_view thumbnailName);
		~ThumbnailFromWorldFactory();

		void Tick() override;

	private:
		uint32 mNumOfTicksReceived{};

		std::string mTextureName{};
		std::unique_ptr<World> mWorld{};
		std::unique_ptr<FrameBuffer> mFrameBuffer{};
	};
}

template<typename T>
std::unique_ptr<CE::ThumbnailFactory> GetThumbNailImpl([[maybe_unused]] const CE::WeakAssetHandle<T>& forAsset)
{
	return std::make_unique<CE::ThumbnailFactory>(CE::GetDefaultThumbnail());
}

template<>
std::unique_ptr<CE::ThumbnailFactory> GetThumbNailImpl(const CE::WeakAssetHandle<CE::Texture>& forAsset);

template<>
std::unique_ptr<CE::ThumbnailFactory> GetThumbNailImpl(const CE::WeakAssetHandle<CE::Script>& forAsset);

template<>
std::unique_ptr<CE::ThumbnailFactory> GetThumbNailImpl(const CE::WeakAssetHandle<CE::Animation>& forAsset);

template<>
std::unique_ptr<CE::ThumbnailFactory> GetThumbNailImpl(const CE::WeakAssetHandle<CE::Level>& forAsset);

template<>
std::unique_ptr<CE::ThumbnailFactory> GetThumbNailImpl(const CE::WeakAssetHandle<CE::Prefab>& forAsset);

template<>
std::unique_ptr<CE::ThumbnailFactory> GetThumbNailImpl(const CE::WeakAssetHandle<CE::StaticMesh>& forAsset);

template<>
std::unique_ptr<CE::ThumbnailFactory> GetThumbNailImpl(const CE::WeakAssetHandle<CE::SkinnedMesh>& forAsset);

template<>
std::unique_ptr<CE::ThumbnailFactory> GetThumbNailImpl(const CE::WeakAssetHandle<CE::Material>& forAsset);

#endif // EDITOR