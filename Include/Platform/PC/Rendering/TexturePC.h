#pragma once
#include "Platform/PC/Rendering/DX12Classes/DXDefines.h"
#include "Assets/Asset.h"
#include "Meta/MetaReflect.h"

class DXDescHeap;
class DXResource;
namespace Engine
{
	class Texture :
		public Asset
	{
		public:
			Texture(std::string_view name);
			Texture(AssetLoadInfo& loadInfo);
			~Texture() override {};

			Texture(Texture&& other) noexcept;
			Texture(const Texture&) = delete;

			Texture& operator=(Texture&&) = delete;
			Texture& operator=(const Texture&) = delete;
			int GetIndex() const { return heapSlot; }

		private:
			bool LoadTexture(const unsigned char* fileContents, const unsigned int width, const unsigned int height, const unsigned int format);
			int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat);
			std::shared_ptr<DXResource> mTextureBuffer;
			int heapSlot = 0;
			friend ReflectAccess;
			static MetaType Reflect();
			REFLECT_AT_START_UP(Texture);
	};
}
