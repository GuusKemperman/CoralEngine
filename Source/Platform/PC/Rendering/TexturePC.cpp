#include "Precomp.h"
#include "Platform/pc/Rendering/TexturePC.h"
#include "Platform/pc/Rendering/DX12Classes/DXDefines.h"
#include "Platform/pc/Rendering/DX12Classes/DXResource.h"
#include "Platform/pc/Rendering/DX12Classes/DXDescHeap.h"

#include "Utilities/StringFunctions.h"
#include "Assets/Core/AssetLoadInfo.h"
#include "stb_image/stb_image.h"
#include "Meta/MetaManager.h"
#include "Utilities/Reflect/ReflectAssetType.h"

#include "Core/Device.h"

Engine::Texture::Texture(AssetLoadInfo& loadInfo) :
	Asset(loadInfo)
{
    const std::string data = StringFunctions::StreamToString(loadInfo.GetStream());

    int width, height, channels;
    unsigned char* pixels = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(data.data()), static_cast<int>(data.size()), &width, &height, &channels, 4);
    if (pixels == nullptr)
    {
        LOG(LogAssets, Error, "Invalid texture {}", GetName());
        return;
    }

	bool textureLoaded = LoadTexture(pixels, static_cast<int>(width), static_cast<int>(height), DXGI_FORMAT_R8G8B8A8_UNORM);

    if (!textureLoaded)
    {
        LOG(LogAssets, Error, "Invalid texture {}", GetName());
    }
}

bool Engine::Texture::LoadTexture(const unsigned char* fileContents, const unsigned int width, const unsigned int height, const unsigned int format)
{
	if (fileContents == nullptr || width <= 0 || height <= 0) {
		return false;
	}

	Device& engineDevice = Device::Get();
	ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());
	ID3D12GraphicsCommandList4* uploadCmdList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetUploadCommandList());

	engineDevice.StartUploadCommands();

	DXGI_FORMAT dxgiformat = (DXGI_FORMAT)format;
	CD3DX12_RESOURCE_DESC resourceDescription = {};
	resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDescription.Alignment = 0;
	resourceDescription.Width = width; 
	resourceDescription.Height = height;
	resourceDescription.DepthOrArraySize = 1;
	resourceDescription.MipLevels = 1;
	resourceDescription.Format = dxgiformat; 
	resourceDescription.SampleDesc.Count = 1; 
	resourceDescription.SampleDesc.Quality = 0;
	resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDescription.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	mTextureBuffer = std::make_shared<DXResource>(device, heapProperties, resourceDescription, nullptr, "Texture Buffer Resource Heap");

	UINT64 textureUploadBufferSize;

	device->GetCopyableFootprints(&resourceDescription, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);

	heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC buffer = CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize);

	int bytesPerRow = (width * GetDXGIFormatBitsPerPixel(resourceDescription.Format)) / 8; // number of bytes in each row of the image data

	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = reinterpret_cast<const void*>(fileContents); // pointer to our image data
	textureData.RowPitch = bytesPerRow; // size of all our triangle vertex data
	textureData.SlicePitch = bytesPerRow * resourceDescription.Height; // also the size of our triangle vertex data

	mTextureBuffer->CreateUploadBuffer(device, static_cast<int>(textureUploadBufferSize), 0);
	mTextureBuffer->Update(uploadCmdList, textureData, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0, 1);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = resourceDescription.Format;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	heapSlot = engineDevice.AllocateTexture(mTextureBuffer.get(), srvDesc);
	engineDevice.SubmitUploadCommands();
	return true;
}

int Engine::Texture::GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat)
{
	if (dxgiFormat == DXGI_FORMAT_R32G32B32A32_FLOAT) return 128;
	else if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_FLOAT) return 64;
	else if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_UNORM) return 64;
	else if (dxgiFormat == DXGI_FORMAT_R8G8B8A8_UNORM) return 32;
	else if (dxgiFormat == DXGI_FORMAT_B8G8R8A8_UNORM) return 32;
	else if (dxgiFormat == DXGI_FORMAT_B8G8R8X8_UNORM) return 32;
	else if (dxgiFormat == DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM) return 32;

	else if (dxgiFormat == DXGI_FORMAT_R10G10B10A2_UNORM) return 32;
	else if (dxgiFormat == DXGI_FORMAT_B5G5R5A1_UNORM) return 16;
	else if (dxgiFormat == DXGI_FORMAT_B5G6R5_UNORM) return 16;
	else if (dxgiFormat == DXGI_FORMAT_R32_FLOAT) return 32;
	else if (dxgiFormat == DXGI_FORMAT_R16_FLOAT) return 16;
	else if (dxgiFormat == DXGI_FORMAT_R16_UNORM) return 16;
	else if (dxgiFormat == DXGI_FORMAT_R8_UNORM) return 8;
	else if (dxgiFormat == DXGI_FORMAT_A8_UNORM) return 8;
	else return 0;
}

Engine::Texture::Texture(Texture&& other) noexcept :
	Asset(std::move(other))
{
	mTextureBuffer = other.mTextureBuffer;
	heapSlot = other.heapSlot;
}
