#pragma once
#include "Assets/Asset.h"
#include "Assets/Core/AssetHandle.h"
#include "Meta/MetaReflect.h"
#include "Utilities/ASync.h"

class DXHeapHandle;
class DXDescHeap;
class DXResource;

namespace Microsoft::WRL
{
	template <typename T>
	class ComPtr;
}

struct ID3D12GraphicsCommandList4;

namespace CE
{
	class FrameBuffer;

	class Texture final :
		public Asset
	{
	public:
		Texture(std::string_view name);

#ifdef EDITOR
		Texture(std::string_view name, FrameBuffer&& frameBuffer);
#endif // EDITOR

		Texture(AssetLoadInfo& loadInfo);
		Texture(std::string_view name, uint32_t width, uint32_t height, const unsigned char* pixels);

		~Texture() override;

		Texture(Texture&& other) noexcept;
		Texture(const Texture&) = delete;

		Texture& operator=(Texture&&) = delete;
		Texture& operator=(const Texture&) = delete;

		static AssetHandle<Texture> TryGetDefaultTexture();

		bool IsReadyToBeSentToGpu() const;
		bool WasSendToGPU() const;
		void SendToGPU() const;

		void BindToGraphics(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4>& commandList, unsigned int rootSlot) const;
		void BindToCompute(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4>& commandList, unsigned int rootSlot) const;

#ifdef EDITOR
		ImTextureID GetImGuiId() const;
#endif // EDITOR

	private:
		void GenerateMipmaps() const;

		const DXHeapHandle* TryGetHeapSlot() const;

		// Prevents having to include the very
		// large DX12 headers
		struct DXImpl;

		struct DXImplDeleter
		{
			void operator()(DXImpl* impl) const;
		}; 

		std::unique_ptr<DXImpl, DXImplDeleter> mImpl{};

		struct STBIPixels
		{
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

		// Stores the return value of the load thread
		// After the texture has been sent to the GPU,
		// this value will be reset to nullptr.
		std::shared_ptr<STBIPixels> mLoadedPixels{};
		ASyncThread mLoadingThread{};

		friend ReflectAccess;
		static MetaType Reflect();
	};
}
