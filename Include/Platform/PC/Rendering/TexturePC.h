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

			/**
			 * \brief Gets the index, but only if it's ready.
			 *
			 * GetIndex will finalise the loading process if the thread has completed it's work.
			 */
			std::optional<int> GetIndex() const;

		private:
			void RetrieveResultsFromLoadThread();

			bool LoadTexture(const unsigned char* fileContents, unsigned int width, unsigned int height, unsigned int format);
			int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat);

			std::shared_ptr<DXResource> mTextureBuffer{};
			int mHeapSlot = -1;

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
			std::thread mLoadThread{};

			friend ReflectAccess;
			static MetaType Reflect();
	};
}
