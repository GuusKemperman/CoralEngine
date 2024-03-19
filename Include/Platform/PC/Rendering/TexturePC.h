#pragma once
#include <thread>

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

			~Texture() override;

			Texture(Texture&& other) noexcept;
			Texture(const Texture&) = delete;

			Texture& operator=(Texture&&) = delete;
			Texture& operator=(const Texture&) = delete;

			bool IsReadyToSendToGPU() const;
			void SendToGPU() const;
			bool WasSendToGPU() const { return mHeapSlot >= 0; }

			int GetIndex() const;

		private:
			int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat);

			std::shared_ptr<DXResource> mTextureBuffer{};

			static constexpr int sAwaitingSendToGPU = -2;
			static constexpr int sFailedToSendToGPU = -1;

			int mHeapSlot = sAwaitingSendToGPU;

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
