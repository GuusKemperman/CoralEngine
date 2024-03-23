#pragma once
#include <thread>

#include "Platform/PC/Rendering/DX12Classes/DXDefines.h"
#include "Platform/PC/Rendering/DX12Classes/DXHeapHandle.h"
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

			~Texture() override;

			Texture(Texture&& other) noexcept;
			Texture(const Texture&) = delete;

			Texture& operator=(Texture&&) = delete;
			Texture& operator=(const Texture&) = delete;

			bool IsReadyToBeSentToGpu() const;
			bool WasSentToGpu() const { return mHeapSlot.has_value(); }
			void SentToGPU() const;

			void BindToGraphics(ComPtr<ID3D12GraphicsCommandList4> commandList, unsigned int rootSlot) const;
			void BindToCompute(ComPtr<ID3D12GraphicsCommandList4> commandList, unsigned int rootSlot) const;

		private:
			int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat);

			std::unique_ptr<DXResource> mTextureBuffer{};
			std::optional<DXHeapHandle> mHeapSlot;

			struct STBIPixels
			{
				~STBIPixels();
				unsigned char* mPixels{};
				int mWidth{};
				int mHeight{};
			};
			// Stores the return value of the load thread
			// After the texture has been sent to the GPU,
			// this value will be reset to nullptr.
			std::shared_ptr<STBIPixels> mLoadedPixels{};

			friend ReflectAccess;
			static MetaType Reflect();
	};
}
