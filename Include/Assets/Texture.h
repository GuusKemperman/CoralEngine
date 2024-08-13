#pragma once
#include "Asset.h"
#include "Core/AssetHandle.h"

namespace CE
{
	struct TexturePlatformImpl;
	class FrameBuffer;

	class Texture :
		public Asset
	{
	public:
		Texture(std::string_view name);
		Texture(std::string_view name, FrameBuffer&& frameBuffer);

		Texture(std::string_view name, std::span<const std::byte> pixelsRGBA, glm::ivec2 size);
		Texture(AssetLoadInfo& loadInfo);

		Texture(Texture&&) noexcept = default;
		Texture(const Texture&) = delete;

		Texture& operator=(Texture&&) = default;
		Texture& operator=(const Texture&) = delete;

		glm::ivec2 GetSize() const { return mSize; }

		const std::shared_ptr<TexturePlatformImpl>& GetPlatformImpl() const { return mImpl; }

		static AssetHandle<Texture> TryGetDefaultTexture();

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(Texture);

		glm::ivec2 mSize{};
		std::shared_ptr<TexturePlatformImpl> mImpl{};
	};
}
