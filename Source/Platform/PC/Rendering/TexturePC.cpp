#include "Precomp.h"
#include "Platform/PC/Rendering/TexturePC.h"
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
#include "Core/JobManager.h"

Engine::Texture::Texture(AssetLoadInfo& loadInfo) :
	Asset(loadInfo),
	mLoadedPixels(std::make_shared<STBIPixels>())
{
	JobManager::Get().AddWork([data = StringFunctions::StreamToString(loadInfo.GetStream()), buffer = mLoadedPixels]()
		{
			int channels{};
			buffer->mPixels = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(data.data()), static_cast<int>(data.size()), &buffer->mWidth, &buffer->mHeight, &channels, 4);
		});
}

Engine::Texture::Texture(Texture&& other) noexcept = default;

Engine::Texture::~Texture() = default;

bool Engine::Texture::IsReadyToBeSentToGpu() const
{
	return !mHeapSlot.has_value()
		&& mLoadedPixels != nullptr
		&& mLoadedPixels->mPixels != nullptr;
}

void Engine::Texture::SendToGPU() const
{
	if (!IsReadyToBeSentToGpu())
	{
		LOG(LogAssets, Error, "{} is not ready to be send to GPU", GetName());
		return;
	}

	Texture& self = const_cast<Texture&>(*this);

	if (mLoadedPixels == nullptr
		|| mLoadedPixels->mPixels == nullptr
		|| mLoadedPixels->mWidth <= 0
		|| mLoadedPixels->mHeight <= 0
		|| Device::IsHeadless())
	{
		LOG(LogAssets, Error, "Invalid texture {}, or device was running headless mode", GetName());
		self.mLoadedPixels.reset();
		return;
	}

	Device& engineDevice = Device::Get();
	ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());
	ID3D12GraphicsCommandList4* uploadCmdList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetUploadCommandList());

	engineDevice.StartUploadCommands();

	DXGI_FORMAT dxgiformat = (DXGI_FORMAT)DXGI_FORMAT_R8G8B8A8_UNORM;
	CD3DX12_RESOURCE_DESC resourceDescription = {};
	resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDescription.Alignment = 0;
	resourceDescription.Width = mLoadedPixels->mWidth;
	resourceDescription.Height = mLoadedPixels->mHeight;
	resourceDescription.DepthOrArraySize = 1;
	resourceDescription.MipLevels = 4;
	resourceDescription.Format = dxgiformat;
	resourceDescription.SampleDesc.Count = 1;
	resourceDescription.SampleDesc.Quality = 0;
	resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDescription.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	self.mTextureBuffer = std::make_unique<DXResource>(device, heapProperties, resourceDescription, nullptr, "Texture Buffer Resource Heap");

	UINT64 textureUploadBufferSize;
	device->GetCopyableFootprints(&resourceDescription, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);

	int bytesPerRow = (mLoadedPixels->mWidth * self.GetDXGIFormatBitsPerPixel(resourceDescription.Format)) / 8; // number of bytes in each row of the image data

	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = mLoadedPixels->mPixels; // pointer to our image data
	textureData.RowPitch = bytesPerRow; // size of all our triangle vertex data
	textureData.SlicePitch = bytesPerRow * resourceDescription.Height; // also the size of our triangle vertex data

	mTextureBuffer->CreateUploadBuffer(device, static_cast<int>(textureUploadBufferSize), 0);
	mTextureBuffer->Update(uploadCmdList, textureData, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0, 1);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = resourceDescription.Format;
	srvDesc.Texture2D.MipLevels = 4;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	self.mHeapSlot = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateResource(mTextureBuffer.get(), &srvDesc);

	for (int i = 1; i < 4; i++) {
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Format = resourceDescription.Format;
		uavDesc.Texture2D.MipSlice = i;
		uavDesc.Texture2D.PlaneSlice = 0;

		self.mUAVslots[i-1] = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateUAV(mTextureBuffer.get(), &uavDesc);
	}

	GenerateMipmaps();

	for (int i = 0; i < 3; i++) {
		self.mUAVslots[i].reset();
	}

	self.mMipmapCB = nullptr;

	engineDevice.SubmitUploadCommands();

	self.mLoadedPixels.reset();
}

void Engine::Texture::BindToGraphics(ComPtr<ID3D12GraphicsCommandList4> commandList, unsigned int rootSlot) const
{
	if (!mHeapSlot.has_value())
	{
		LOG(LogAssets, Error, "{} has not been send to GPU", GetName());
		return;
	}

	commandList->SetGraphicsRootDescriptorTable(rootSlot, mHeapSlot->GetAddressGPU());
}

void Engine::Texture::BindToCompute(ComPtr<ID3D12GraphicsCommandList4> commandList, unsigned int rootSlot) const
{
	if (!mHeapSlot.has_value())
	{
		LOG(LogAssets, Error, "{} has not been send to GPU", GetName());
		return;
	}

	commandList->SetComputeRootDescriptorTable(rootSlot, mHeapSlot->GetAddressGPU());
}

int Engine::Texture::GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat)
{
	switch (dxgiFormat)
	{
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
		return 128;
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
		return 64;
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
	case DXGI_FORMAT_R10G10B10A2_UNORM:
	case DXGI_FORMAT_R32_FLOAT:
		return 32;
	case DXGI_FORMAT_B5G5R5A1_UNORM:
	case DXGI_FORMAT_B5G6R5_UNORM:
	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_R16_UNORM:
		return 16;
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_A8_UNORM:
		return 8;
	default:
		return 0;
	}
}

void Engine::Texture::GenerateMipmaps() const
{
	//FROM: https://www.3dgep.com/learning-directx-12-4/#Generate_Mipmaps_Compute_Shader
	//TODO: Do the required steps if resource does not support UAVs
	Texture& self = const_cast<Texture&>(*this);

	Device& engineDevice = Device::Get();
	ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());
	ID3D12GraphicsCommandList4* uploadCmdList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetUploadCommandList());

	DXGenerateMips generateMipsCB;
	generateMipsCB.IsSRGB = false; //TODO: check if format is SRGB

	auto resource = self.mTextureBuffer->GetResource();
	auto resourceDesc = resource->GetDesc();

	if (resourceDesc.Width <= 4 || resourceDesc.Height <=4)
		return;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = resourceDesc.Format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;  // Only 2D textures are supported (this was checked in the calling function).
	srvDesc.Texture2D.MipLevels = resourceDesc.MipLevels;

	CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	self.mMipmapCB = std::make_unique<DXConstBuffer>(device, sizeof(DXGenerateMips), 1, "MIPMAP CREATION INFO BUFFER", FRAME_BUFFER_COUNT);


	uint64_t srcWidth = resourceDesc.Width;
	uint32_t srcHeight = resourceDesc.Height;
	uint32_t dstWidth = static_cast<uint32_t>(srcWidth >> 1);
	uint32_t dstHeight = srcHeight >> 1;

	// 0b00(0): Both width and height are even.
	// 0b01(1): Width is odd, height is even.
	// 0b10(2): Width is even, height is odd.
	// 0b11(3): Both width and height are odd.
	generateMipsCB.SrcDimension = ( srcHeight & 1 ) << 1 | ( srcWidth & 1 );
	DWORD mipCount = 4;

	// The number of times we can half the size of the texture and get
	// exactly a 50% reduction in size.
	// A 1 bit in the width or height indicates an odd dimension.
	// The case where either the width or the height is exactly 1 is handled
	// as a special case (as the dimension does not require reduction).
	_BitScanForward( &mipCount, ( dstWidth == 1 ? dstHeight : dstWidth ) | 
		( dstHeight == 1 ? dstWidth : dstHeight ) );

	// Dimensions should not reduce to 0.
	// This can happen if the width and height are not the same.
	dstWidth = std::max<DWORD>( 1, dstWidth );
	dstHeight = std::max<DWORD>( 1, dstHeight );

	generateMipsCB.SrcMipLevel = 0;
	generateMipsCB.NumMipLevels = mipCount;
	generateMipsCB.TexelSize.x = 1.0f / (float)dstWidth;
	generateMipsCB.TexelSize.y = 1.0f / (float)dstHeight;
	self.mMipmapCB->Update(&generateMipsCB, sizeof(DXGenerateMips), 0, engineDevice.GetFrameIndex());

	uploadCmdList->SetPipelineState(reinterpret_cast<DXPipeline*>(engineDevice.GetMipmapPipeline())->GetPipeline().Get());
	uploadCmdList->SetComputeRootSignature(reinterpret_cast<DXSignature*>(engineDevice.GetComputeSignature())->GetSignature().Get());
	ID3D12DescriptorHeap* descriptorHeaps[] = {engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->Get()};
	uploadCmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	self.mMipmapCB->BindToCompute(uploadCmdList, 0, 0, engineDevice.GetFrameIndex());
	engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->BindToCompute(uploadCmdList, 10, *mHeapSlot);
	engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->BindToCompute(uploadCmdList, 6, *mUAVslots[0]);
	engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->BindToCompute(uploadCmdList, 7, *mUAVslots[1]);
	engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->BindToCompute(uploadCmdList, 8, *mUAVslots[2]);

	uploadCmdList->Dispatch(dstWidth /8, dstHeight / 8,  1);
}

Engine::Texture::STBIPixels::~STBIPixels()
{
	stbi_image_free(mPixels);
}
