#pragma once
#include "Assets/Asset.h"
#include "Meta/MetaReflect.h"

class DXDescHeap;
class DXResource;
enum DXGI_FORMAT;

namespace Engine
{
	class Texture :
		public Asset
	{
		public:
			Texture(std::string_view name);
			Texture(AssetLoadInfo& loadInfo);
			~Texture() override;

			Texture(Texture&& other) noexcept;
			Texture(const Texture&) = delete;

			Texture& operator=(Texture&&) = delete;
			Texture& operator=(const Texture&) = delete;

		private:
			bool LoadTexture(const unsigned char* fileContents, const unsigned int width, const unsigned int height, const unsigned int format);
			int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat);
			std::shared_ptr<DXResource> mTextureBuffer;

			friend ReflectAccess;
			static MetaType Reflect();
			REFLECT_AT_START_UP(Texture);
	};
}
