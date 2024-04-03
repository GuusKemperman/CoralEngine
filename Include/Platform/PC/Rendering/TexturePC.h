#pragma once
#include <future>
#include <thread>

#include "Platform/PC/Rendering/DX12Classes/DXDefines.h"
#include "Platform/PC/Rendering/DX12Classes/DXHeapHandle.h"
#include "Platform/PC/Rendering/DX12Classes/DXConstBuffer.h"
#include "Assets/Asset.h"
#include "Meta/MetaReflect.h"

class DXDescHeap;
class DXResource;

namespace CE
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

			bool IsReadyToBeSentToGpu() const;
			bool WasSentToGpu() const { return mHeapSlot.has_value(); }
			void SendToGPU() const;

			void BindToGraphics(ComPtr<ID3D12GraphicsCommandList4> commandList, unsigned int rootSlot) const;
			void BindToCompute(ComPtr<ID3D12GraphicsCommandList4> commandList, unsigned int rootSlot) const;

		private:
			int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat);
			void GenerateMipmaps() const;
			std::unique_ptr<DXResource> mTextureBuffer{};
			std::optional<DXHeapHandle> mHeapSlot;

			std::optional<DXHeapHandle> mUAVslots[3];
			std::unique_ptr<DXConstBuffer> mMipmapCB;

			struct STBIPixels
			{
				STBIPixels() = default;
				STBIPixels(STBIPixels&& pixels);
				STBIPixels(const STBIPixels&) = delete;

				STBIPixels& operator=(STBIPixels&&);
				STBIPixels& operator=(const STBIPixels&) = delete;

				~STBIPixels();



				unsigned char* mPixels{};
				int mWidth{};
				int mHeight{};
			};

			struct DXGenerateMips
			{
				uint32_t SrcMipLevel;           // Texture level of source mip
				uint32_t NumMipLevels;          // Number of OutMips to write: [1-4]
				uint32_t SrcDimension;          // Width and height of the source texture are even or odd.
				uint32_t IsSRGB;                // Must apply gamma correction to sRGB textures.
				glm::vec2 TexelSize;			// 1.0 / OutMip1.Dimensions
			};

			std::future<STBIPixels> mLoadedPixels{};

			friend ReflectAccess;
			static MetaType Reflect();
	};
}
