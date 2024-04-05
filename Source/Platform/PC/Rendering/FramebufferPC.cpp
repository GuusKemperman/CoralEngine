#include "Precomp.h"
#ifdef EDITOR
#include "Platform/PC/Rendering/FramebufferPC.h"
#include "Core/Device.h"
#include "Platform/PC/Rendering/DX12Classes/DXDescHeap.h"
#include "Platform/PC/Rendering/DX12Classes/DXResource.h"
#include "Platform/PC/Rendering/DX12Classes/DXSignature.h"

CE::FrameBuffer::FrameBuffer(glm::ivec2 initialSize)
{
	if (Device::IsHeadless())
	{
		return;
	}

	Device& engineDevice = Device::Get();
	ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());
	mSize = initialSize;

	mViewport.Width = static_cast<FLOAT>(mSize.x);
	mViewport.Height = static_cast<FLOAT>(mSize.y);
	mViewport.TopLeftX = 0;
	mViewport.TopLeftY = 0;
	mViewport.MinDepth = 0.0f;
	mViewport.MaxDepth = 1.0f;

	mScissorRect.left = 0;
	mScissorRect.top = 0;
	mScissorRect.right = static_cast<LONG>(mViewport.Width);
	mScissorRect.bottom = static_cast<LONG>(mViewport.Height);

	mClearColor = { 0.2f, 0.2f, 0.2f, 1.f };
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Use the format that matches your RTV format.
	clearValue.Color[0] = mClearColor.x; // Red component
	clearValue.Color[1] = mClearColor.y; // Green component
	clearValue.Color[2] = mClearColor.z; // Blue component
	clearValue.Color[3] = mClearColor.w; // Alpha component

	auto framebufferDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, static_cast<UINT>(mSize.x), static_cast<UINT>(mSize.y), 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	for (int i = 0; i < FRAME_BUFFER_COUNT; i++) {
		mResource[i] = std::make_unique<DXResource>(device, heapProperties, framebufferDesc, &clearValue, "Framebuffer");

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

		mFrameBufferHandle[i] = engineDevice.GetDescriptorHeap(RT_HEAP)->AllocateRenderTarget(mResource[i].get(), &rtvDesc);
		mFrameBufferRscHandle[i] = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateResource(mResource[i].get(), &srvDesc);
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, static_cast<UINT>(mSize.x), static_cast<UINT>(mSize.y), 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	mDepthResource = std::make_unique<DXResource>(device, heapProperties, resourceDesc, &depthOptimizedClearValue, "Depth/Stencil Resource");
	mDepthStencilHandle = engineDevice.GetDescriptorHeap(DEPTH_HEAP)->AllocateDepthStencil(mDepthResource.get(), &depthStencilDesc);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

	mDepthStencilSRVHandle = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateResource(mDepthResource.get(), &srvDesc);
}

CE::FrameBuffer::~FrameBuffer() = default;

void CE::FrameBuffer::Bind()
{
	Device& engineDevice = Device::Get();
	std::shared_ptr<DXDescHeap> rtHeap = engineDevice.GetDescriptorHeap(RT_HEAP);
	ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
	mResource[engineDevice.GetFrameIndex()]->ChangeState(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	mDepthResource->ChangeState(commandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	commandList->RSSetViewports(1, &mViewport);
	commandList->RSSetScissorRects(1, &mScissorRect);
	rtHeap->BindRenderTargets(commandList, &mFrameBufferHandle[engineDevice.GetFrameIndex()], mDepthStencilHandle);
}

void CE::FrameBuffer::Unbind()
{

}

void CE::FrameBuffer::Resize(glm::ivec2 newSize)
{
	if (mSize == newSize)
	{
		return;
	}

	mSize = newSize;

	if (mSize.x <= 0 || mSize.y <= 0)
	{
		return;
	}

	mViewport.Width = static_cast<FLOAT>(mSize.x);
	mViewport.Height = static_cast<FLOAT>(mSize.y);
	mViewport.TopLeftX = 0;
	mViewport.TopLeftY = 0;
	mViewport.MinDepth = 0.0f;
	mViewport.MaxDepth = 1.0f;

	mScissorRect.left = 0;
	mScissorRect.top = 0;
	mScissorRect.right = static_cast<LONG>(mViewport.Width);
	mScissorRect.bottom = static_cast<LONG>(mViewport.Height);

	Device& engineDevice = Device::Get();
	ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());

	mClearColor = { 0.2f, 0.2f, 0.2f, 1.f };
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Use the format that matches your RTV format.
	clearValue.Color[0] = mClearColor.x; // Red component
	clearValue.Color[1] = mClearColor.y; // Green component
	clearValue.Color[2] = mClearColor.z; // Blue component
	clearValue.Color[3] = mClearColor.w; // Alpha component

	auto framebufferDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, static_cast<UINT>(mSize.x), static_cast<UINT>(mSize.y), 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	for (int i = 0; i < FRAME_BUFFER_COUNT; i++) {
		mResource[i] = std::make_unique<DXResource>(device, heapProperties, framebufferDesc, &clearValue, "Framebuffer resource");

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		mFrameBufferHandle[i] = engineDevice.GetDescriptorHeap(RT_HEAP)->AllocateRenderTarget(mResource[i].get(), &rtvDesc);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

		mFrameBufferRscHandle[i] = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateResource(mResource[i].get(), &srvDesc);
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, static_cast<UINT>(mSize.x), static_cast<UINT>(mSize.y), 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	mDepthResource = std::make_unique<DXResource>(device, heapProperties, resourceDesc, &depthOptimizedClearValue, "Framebuffer depth Resource");
	mDepthStencilHandle = engineDevice.GetDescriptorHeap(DEPTH_HEAP)->AllocateDepthStencil(mDepthResource.get(), &depthStencilDesc);
}

void CE::FrameBuffer::Clear()
{
	Device& engineDevice = Device::Get();
	std::shared_ptr<DXDescHeap> rtHeap = engineDevice.GetDescriptorHeap(RT_HEAP);
	std::shared_ptr<DXDescHeap> depthHeap = engineDevice.GetDescriptorHeap(DEPTH_HEAP);
	ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
	rtHeap->ClearRenderTarget(commandList, mFrameBufferHandle[engineDevice.GetFrameIndex()], &mClearColor[0]);
	depthHeap->ClearDepthStencil(commandList, mDepthStencilHandle);
}

size_t CE::FrameBuffer::GetColorTextureId()
{
	Device& engineDevice = Device::Get();
	return mFrameBufferRscHandle[engineDevice.GetFrameIndex()].GetAddressGPU().ptr;
}
#endif