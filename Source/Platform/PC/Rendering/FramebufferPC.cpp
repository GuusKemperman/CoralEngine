#include "Precomp.h"
#include "Platform/PC/Rendering/FramebufferPC.h"
#include "Core/Device.h"
#include "Platform/PC/Rendering/DX12Classes/DXDescHeap.h"

Engine::FrameBuffer::FrameBuffer(glm::ivec2 initialSize)
{
	Device& engineDevice = Device::Get();
	ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());
	mSize = initialSize;

	mClearColor = { 0.2f, 0.2f, 0.2f, 1.f };
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Use the format that matches your RTV format.
	clearValue.Color[0] = mClearColor.x; // Red component
	clearValue.Color[1] = mClearColor.y; // Green component
	clearValue.Color[2] = mClearColor.z; // Blue component
	clearValue.Color[3] = mClearColor.w; // Alpha component

	CD3DX12_RESOURCE_DESC framebufferDesc = {};
	framebufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	framebufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Adjust the format as needed.
	framebufferDesc.Width = initialSize.x;
	framebufferDesc.Height = initialSize.y;
	framebufferDesc.DepthOrArraySize = 1;
	framebufferDesc.MipLevels = 1;
	framebufferDesc.SampleDesc.Count = 1; 
	framebufferDesc.SampleDesc.Quality = 0;
	framebufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	framebufferDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	framebufferDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;

	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	for (int i = 0; i < FRAME_BUFFER_COUNT; i++) {
		resource[i] = std::make_unique<DXResource>(device, heapProperties, framebufferDesc, &clearValue, "Framebuffer");

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		frameBufferIndex[i] = engineDevice.AllocateFramebuffer(resource[i].get(), rtvDesc);
	}
}

Engine::FrameBuffer::~FrameBuffer()
{

}

void Engine::FrameBuffer::Bind()
{
	Device& engineDevice = Device::Get();
	std::shared_ptr<DXDescHeap> rtHeap = engineDevice.GetDescriptorHeap(RT_HEAP);
	ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
	CD3DX12_CPU_DESCRIPTOR_HANDLE depthHandle = engineDevice.GetDescriptorHeap(DEPTH_HEAP)->GetCPUHandle(0);
	rtHeap->BindRenderTargets(commandList, &frameBufferIndex[engineDevice.GetFrameIndex()], depthHandle);
}

void Engine::FrameBuffer::Unbind()
{

}

void Engine::FrameBuffer::Resize(glm::ivec2 newSize)
{
	Device& engineDevice = Device::Get();
	ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());

	mClearColor = { 0.2f, 0.2f, 0.2f, 1.f };
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Use the format that matches your RTV format.
	clearValue.Color[0] = mClearColor.x; // Red component
	clearValue.Color[1] = mClearColor.y; // Green component
	clearValue.Color[2] = mClearColor.z; // Blue component
	clearValue.Color[3] = mClearColor.w; // Alpha component

	CD3DX12_RESOURCE_DESC framebufferDesc = {};
	framebufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	framebufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Adjust the format as needed.
	framebufferDesc.Width = newSize.x;
	framebufferDesc.Height = newSize.y;
	framebufferDesc.DepthOrArraySize = 1;
	framebufferDesc.MipLevels = 1;
	framebufferDesc.SampleDesc.Count = 1; 
	framebufferDesc.SampleDesc.Quality = 0;
	framebufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	framebufferDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	framebufferDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;

	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	for (int i = 0; i < FRAME_BUFFER_COUNT; i++) {
		resource[i] = std::make_unique<DXResource>(device, heapProperties, framebufferDesc, &clearValue, "Framebuffer");

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		engineDevice.AllocateFramebuffer(resource[i].get(), rtvDesc, frameBufferIndex[i]);
	}
}

void Engine::FrameBuffer::Clear()
{
	Device& engineDevice = Device::Get();
	std::shared_ptr<DXDescHeap> rtHeap = engineDevice.GetDescriptorHeap(RT_HEAP);
	ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
	rtHeap->ClearRenderTarget(commandList, frameBufferIndex[engineDevice.GetFrameIndex()], &mClearColor[0]);
}
