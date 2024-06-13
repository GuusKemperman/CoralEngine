#include "Precomp.h"
#include "Platform/PC/Rendering/FramebufferPC.h"
#include "Core/Device.h"
#include "Platform/PC/Rendering/DX12Classes/DXDescHeap.h"
#include "Platform/PC/Rendering/DX12Classes/DXResource.h"
#include "Platform/PC/Rendering/DX12Classes/DXHeapHandle.h"

struct CE::FrameBuffer::DXImpl
{
	std::unique_ptr<DXResource> mResource[FRAME_BUFFER_COUNT]{};
	std::unique_ptr<DXResource> mDepthResource{};
	DXHeapHandle mFrameBufferHandle[FRAME_BUFFER_COUNT]{};
	DXHeapHandle mFrameBufferRscHandle[FRAME_BUFFER_COUNT]{};
	DXHeapHandle mFrameBufferUAVHandle[FRAME_BUFFER_COUNT]{};
	DXHeapHandle mDepthStencilHandle{};
	DXHeapHandle mDepthStencilSRVHandle{};
	D3D12_VIEWPORT mViewport{};
	D3D12_RECT mScissorRect{};
	DXGI_FORMAT mRTFormat;
};

CE::FrameBuffer::FrameBuffer(glm::ivec2 initialSize, uint32 msaaCount, uint32 msaaQuality, bool floatingPoint) :
	mImpl(new DXImpl())
{
	if (Device::IsHeadless())
	{
		return;
	}

	mMsaaCount = msaaCount;
	mMsaaQuality = msaaQuality;
	Device& engineDevice = Device::Get();
	ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());
	auto rscHeap = engineDevice.GetDescriptorHeap(RESOURCE_HEAP);
	auto rtHeap = engineDevice.GetDescriptorHeap(RT_HEAP);
	auto depthHeap = engineDevice.GetDescriptorHeap(DEPTH_HEAP);

	mSize = glm::vec2(initialSize.x, initialSize.y);
	mImpl->mViewport.Width = static_cast<FLOAT>(mSize.x);
	mImpl->mViewport.Height = static_cast<FLOAT>(mSize.y);
	mImpl->mViewport.TopLeftX = 0;
	mImpl->mViewport.TopLeftY = 0;
	mImpl->mViewport.MinDepth = 0.0f;
	mImpl->mViewport.MaxDepth = 1.0f;

	mImpl->mScissorRect.left = 0;
	mImpl->mScissorRect.top = 0;
	mImpl->mScissorRect.right = static_cast<LONG>(mImpl->mViewport.Width);
	mImpl->mScissorRect.bottom = static_cast<LONG>(mImpl->mViewport.Height);

	mImpl->mRTFormat = floatingPoint ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM;
	
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = mImpl->mRTFormat; // Use the format that matches your RTV format.
	clearValue.Color[0] = mClearColor.x; // Red component
	clearValue.Color[1] = mClearColor.y; // Green component
	clearValue.Color[2] = mClearColor.z; // Blue component
	clearValue.Color[3] = mClearColor.w; // Alpha component

	auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(mImpl->mRTFormat, initialSize.x, initialSize.y, 1, 1, msaaCount, msaaQuality, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	for (int i = 0; i < FRAME_BUFFER_COUNT; i++) 
	{
		mImpl->mResource[i] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, &clearValue, "Framebuffer");

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = mImpl->mRTFormat;
		if(msaaCount>1)
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
		else
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = mImpl->mRTFormat;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		if(msaaCount>1)
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
		else
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

		mImpl->mFrameBufferHandle[i] = rtHeap->AllocateRenderTarget(mImpl->mResource[i].get(), &rtvDesc);
		mImpl->mFrameBufferRscHandle[i] = rscHeap->AllocateResource(mImpl->mResource[i].get(), &srvDesc);
	}

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;
	heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, initialSize.x, initialSize.y, 1, 1, msaaCount, msaaQuality, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	mImpl->mDepthResource = std::make_unique<DXResource>(device, heapProperties, resourceDesc, &depthOptimizedClearValue, "Depth/Stencil Resource");
	
	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	if(msaaCount>1)
		depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
	else
		depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;
	mImpl->mDepthStencilHandle = depthHeap->AllocateDepthStencil(mImpl->mDepthResource.get(), &depthStencilDesc);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	if(msaaCount>1)
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
	else
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

	mImpl->mDepthStencilSRVHandle = rscHeap->AllocateResource(mImpl->mDepthResource.get(), &srvDesc);
}

CE::FrameBuffer::~FrameBuffer() = default;

void CE::FrameBuffer::Bind() const
{
	Device& engineDevice = Device::Get();
	std::shared_ptr<DXDescHeap> rtHeap = engineDevice.GetDescriptorHeap(RT_HEAP);
	ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
	mImpl->mResource[engineDevice.GetFrameIndex()]->ChangeState(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	mImpl->mDepthResource->ChangeState(commandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	commandList->RSSetViewports(1, &mImpl->mViewport);
	commandList->RSSetScissorRects(1, &mImpl->mScissorRect);
	rtHeap->BindRenderTargets(commandList, &mImpl->mFrameBufferHandle[engineDevice.GetFrameIndex()], mImpl->mDepthStencilHandle);
}

void CE::FrameBuffer::Unbind() const
{

}

void CE::FrameBuffer::ResolveMsaa(FrameBuffer& msaaFramebuffer)
{
	Device& engineDevice = Device::Get();
	ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
	if (msaaFramebuffer.mSize != mSize)
		return;

	mImpl->mResource[engineDevice.GetFrameIndex()]->ChangeState(commandList, D3D12_RESOURCE_STATE_RESOLVE_DEST);
	msaaFramebuffer.PrepareMsaaForResolve();

	commandList->ResolveSubresource(
		mImpl->mResource[engineDevice.GetFrameIndex()]->Get(),
		0,
		msaaFramebuffer.GetResource().Get(),
		0,
		mImpl->mRTFormat);
}

void CE::FrameBuffer::PrepareMsaaForResolve()
{
	Device& engineDevice = Device::Get();
	ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
	mImpl->mResource[engineDevice.GetFrameIndex()]->ChangeState(commandList, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
}

void CE::FrameBuffer::BindSRVDepthToGraphics(int rootSlot) const
{
	Device& engineDevice = Device::Get();
	std::shared_ptr<DXDescHeap> resourceHeap = engineDevice.GetDescriptorHeap(RESOURCE_HEAP);
	ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());

	mImpl->mDepthResource->ChangeState(commandList, D3D12_RESOURCE_STATE_GENERIC_READ);
	resourceHeap->BindToGraphics(commandList, rootSlot, mImpl->mDepthStencilSRVHandle);
}

void CE::FrameBuffer::BindSRVRTToGraphics(int rootSlot) const
{
	Device& engineDevice = Device::Get();
	std::shared_ptr<DXDescHeap> resourceHeap = engineDevice.GetDescriptorHeap(RESOURCE_HEAP);
	ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());

	mImpl->mResource[engineDevice.GetFrameIndex()]->ChangeState(commandList, D3D12_RESOURCE_STATE_GENERIC_READ);
	resourceHeap->BindToGraphics(commandList, rootSlot, mImpl->mFrameBufferRscHandle[engineDevice.GetFrameIndex()]);

}

void CE::FrameBuffer::Resize(glm::ivec2 newSize)
{
	if (mSize == static_cast<glm::vec2>(newSize))
	{
		return;
	}

	mSize = newSize;

	if (mSize.x <= 0 || mSize.y <= 0)
	{
		return;
	}

	mImpl->mViewport.Width = static_cast<FLOAT>(mSize.x);
	mImpl->mViewport.Height = static_cast<FLOAT>(mSize.y);
	mImpl->mViewport.TopLeftX = 0;
	mImpl->mViewport.TopLeftY = 0;
	mImpl->mViewport.MinDepth = 0.0f;
	mImpl->mViewport.MaxDepth = 1.0f;

	mImpl->mScissorRect.left = 0;
	mImpl->mScissorRect.top = 0;
	mImpl->mScissorRect.right = static_cast<LONG>(mImpl->mViewport.Width);
	mImpl->mScissorRect.bottom = static_cast<LONG>(mImpl->mViewport.Height);

	Device& engineDevice = Device::Get();
	ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());
	auto rscHeap = engineDevice.GetDescriptorHeap(RESOURCE_HEAP);
	auto rtHeap = engineDevice.GetDescriptorHeap(RT_HEAP);
	auto depthHeap = engineDevice.GetDescriptorHeap(DEPTH_HEAP);

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = mImpl->mRTFormat; // Use the format that matches your RTV format.
	clearValue.Color[0] = mClearColor.x; // Red component
	clearValue.Color[1] = mClearColor.y; // Green component
	clearValue.Color[2] = mClearColor.z; // Blue component
	clearValue.Color[3] = mClearColor.w; // Alpha component

	auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(mImpl->mRTFormat, newSize.x, newSize.y, 1, 1, mMsaaCount, mMsaaQuality, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	for (int i = 0; i < FRAME_BUFFER_COUNT; i++) 
	{
		mImpl->mResource[i] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, &clearValue, "Framebuffer");

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = mImpl->mRTFormat;
		if(mMsaaCount>1)
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
		else
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = mImpl->mRTFormat;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		if(mMsaaCount>1)
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
		else
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

		mImpl->mFrameBufferHandle[i] = rtHeap->AllocateRenderTarget(mImpl->mResource[i].get(), &rtvDesc);
		mImpl->mFrameBufferRscHandle[i] = rscHeap->AllocateResource(mImpl->mResource[i].get(), &srvDesc);
	}

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;
	heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, newSize.x, newSize.y, 1, 1, mMsaaCount, mMsaaQuality, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	mImpl->mDepthResource = std::make_unique<DXResource>(device, heapProperties, resourceDesc, &depthOptimizedClearValue, "Depth/Stencil Resource");

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	if(mMsaaCount>1)
		depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
	else
		depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;
	mImpl->mDepthStencilHandle = depthHeap->AllocateDepthStencil(mImpl->mDepthResource.get(), &depthStencilDesc);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	if(mMsaaCount>1)
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
	else
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

	mImpl->mDepthStencilSRVHandle = rscHeap->AllocateResource(mImpl->mDepthResource.get(), &srvDesc);
}


void CE::FrameBuffer::Clear()
{
	Device& engineDevice = Device::Get();
	std::shared_ptr<DXDescHeap> rtHeap = engineDevice.GetDescriptorHeap(RT_HEAP);
	std::shared_ptr<DXDescHeap> depthHeap = engineDevice.GetDescriptorHeap(DEPTH_HEAP);
	ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
	rtHeap->ClearRenderTarget(commandList, mImpl->mFrameBufferHandle[engineDevice.GetFrameIndex()], &mClearColor[0]);
	depthHeap->ClearDepthStencil(commandList, mImpl->mDepthStencilHandle);
}

DXResource& CE::FrameBuffer::GetResource() const
{
	Device& engineDevice = Device::Get();
	return *mImpl->mResource[engineDevice.GetFrameIndex()].get();
}

size_t CE::FrameBuffer::GetColorTextureId()
{
	Device& engineDevice = Device::Get();
	return mImpl->mFrameBufferRscHandle[engineDevice.GetFrameIndex()].GetAddressGPU().ptr;
}

DXHeapHandle& CE::FrameBuffer::GetCurrentHeapSlot()
{
	return mImpl->mFrameBufferRscHandle[Device::Get().GetFrameIndex()];
}

std::unique_ptr<DXResource>& CE::FrameBuffer::GetCurrentResource()
{
	return mImpl->mResource[Device::Get().GetFrameIndex()];
}

void CE::FrameBuffer::DXImplDeleter::operator()(DXImpl* impl) const
{
	delete impl;
}
