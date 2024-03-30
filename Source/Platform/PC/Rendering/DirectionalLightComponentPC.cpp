#include "Precomp.h"

#include "Components/DirectionalLightComponent.h"
#include "Platform/PC/Rendering/DX12Classes/DXResource.h"
#include "Platform/PC/Rendering/DX12Classes/DXHeapHandle.h"
#include "Core/Device.h"

class Engine::DirectionalLightComponent::Impl
{
public:
	Impl();
	~Impl() = default;

	std::unique_ptr<DXResource> mDepthResource;
	std::unique_ptr<DXResource> mRenderTarget;
	DXHeapHandle mDepthHandle;
	DXHeapHandle mRTHandle;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;
};


Engine::DirectionalLightComponent::DirectionalLightComponent() {
	mImpl = std::make_unique<Impl>();
}

Engine::DirectionalLightComponent::~DirectionalLightComponent()
{
}

void Engine::DirectionalLightComponent::BindDepthResource() const {
	Device& engineDevice = Device::Get();
	std::shared_ptr<DXDescHeap> rtHeap = engineDevice.GetDescriptorHeap(RT_HEAP);
	std::shared_ptr<DXDescHeap> depthHeap = engineDevice.GetDescriptorHeap(DEPTH_HEAP);
	glm::vec4 clearColor = glm::vec4(0.f);

	ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
	depthHeap->ClearDepthStencil(commandList, mImpl->mDepthHandle);
	rtHeap->ClearRenderTarget(commandList, mImpl->mRTHandle, &clearColor[0]);

	mImpl->mRenderTarget->ChangeState(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	mImpl->mDepthResource->ChangeState(commandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	commandList->RSSetViewports(1, &mImpl->mViewport);
	commandList->RSSetScissorRects(1, &mImpl->mScissorRect);
	rtHeap->BindRenderTargets(commandList, &mImpl->mRTHandle, mImpl->mDepthHandle);
}

Engine::DirectionalLightComponent::Impl::Impl() {

	Device& engineDevice = Device::Get();
	ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, 2048, 2048, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	mDepthResource = std::make_unique<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), resourceDesc, &depthOptimizedClearValue, "DIRECTIONAL LIGHT DEPTH STENCIL");

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;
	mDepthHandle = engineDevice.GetDescriptorHeap(DEPTH_HEAP)->AllocateDepthStencil(mDepthResource.get(), &depthStencilDesc);

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Use the format that matches your RTV format.
	clearValue.Color[0] = 0.f; // Red component
	clearValue.Color[1] = 0.f; // Green component
	clearValue.Color[2] = 0.f; // Blue component
	clearValue.Color[3] = 0.f; // Alpha component
	resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 2048, 2048, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	mRenderTarget = std::make_unique<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), resourceDesc, &clearValue, "DIRECTIONAL LIGHT RENDER TARGET");

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	mRTHandle = engineDevice.GetDescriptorHeap(RT_HEAP)->AllocateRenderTarget(mRenderTarget.get(), &rtvDesc);

	mViewport.Width = static_cast<FLOAT>(2048);
	mViewport.Height = static_cast<FLOAT>(2048);
	mViewport.TopLeftX = 0;
	mViewport.TopLeftY = 0;
	mViewport.MinDepth = 0.0f;
	mViewport.MaxDepth = 1.0f;

	mScissorRect.left = 0;
	mScissorRect.top = 0;
	mScissorRect.right = static_cast<LONG>(mViewport.Width);
	mScissorRect.bottom = static_cast<LONG>(mViewport.Height);
}