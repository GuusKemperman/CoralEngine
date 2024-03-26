#include "Precomp.h"
#include "Platform/PC/Rendering/DX12Classes/DXDescHeap.h"
#include "Platform/PC/Rendering/DX12Classes/DXHeapHandle.h"
#include "Core/Device.h"

DXDescHeap::DXDescHeap(const ComPtr<ID3D12Device5>& device, int numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type, LPCWSTR name, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
{
	D3D12_DESCRIPTOR_HEAP_DESC renderTargetDesc = {};
	renderTargetDesc.NumDescriptors = numDescriptors;
	renderTargetDesc.Type = type;
	renderTargetDesc.Flags = flags;

	mType = type;
	mMaxResources = numDescriptors;

	//Leaving space for ImGUI resources
	if (mType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
		mResourceCount = RESOURCE_START;

	HRESULT hr = device->CreateDescriptorHeap(&renderTargetDesc, IID_PPV_ARGS(&mDescriptorHeap));
	if (FAILED(hr))
	{
		LOG(LogCore, Fatal, "Failed to create descriptor heap");
		assert(false && "Failed to create descriptor heap");
	}
	mDescriptorSize = device->GetDescriptorHandleIncrementSize(type);
	mDescriptorHeap->SetName(name);
}

void DXDescHeap::BindToGraphics(ComPtr<ID3D12GraphicsCommandList4> commandList, unsigned int rootSlot, const DXHeapHandle &handle)
{
	commandList->SetGraphicsRootDescriptorTable(rootSlot, handle.GetAddressGPU());
}

void DXDescHeap::BindToCompute(ComPtr<ID3D12GraphicsCommandList4> commandList, unsigned int rootSlot, const DXHeapHandle& handle)
{
	commandList->SetComputeRootDescriptorTable(rootSlot, handle.GetAddressGPU());
}

void DXDescHeap::BindRenderTargets(ComPtr<ID3D12GraphicsCommandList4> commandList, const DXHeapHandle* handles, const DXHeapHandle& dsvHandle, unsigned int numRtv)
{
	std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> rtvHandles(numRtv);

	for (unsigned int i = 0; i < numRtv; i++)
		rtvHandles[i] = handles[i].GetAddressCPU();

	CD3DX12_CPU_DESCRIPTOR_HANDLE depthHandle = dsvHandle.GetAddressCPU();
	commandList->OMSetRenderTargets(static_cast<UINT>(rtvHandles.size()), rtvHandles.data(), FALSE, &depthHandle);
}

void DXDescHeap::BindRenderTargets(ComPtr<ID3D12GraphicsCommandList4> commandList, const DXHeapHandle& handle) {

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtHandle = handle.GetAddressCPU();
	commandList->OMSetRenderTargets(1, &rtHandle, FALSE, nullptr);
}


void DXDescHeap::ClearRenderTarget(ComPtr<ID3D12GraphicsCommandList4> commandList, const DXHeapHandle& handle, const float* clearData)
{
	if (mType != D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
		return;

	commandList->ClearRenderTargetView(handle.GetAddressCPU(), clearData, 0, nullptr);
}

void DXDescHeap::ClearDepthStencil(ComPtr<ID3D12GraphicsCommandList4> commandList, const DXHeapHandle& handle)
{
	if (mType != D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
		return;

	commandList->ClearDepthStencilView(handle.GetAddressCPU(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

DXHeapHandle DXDescHeap::AllocateResource(DXResource* resource, D3D12_SHADER_RESOURCE_VIEW_DESC* desc)
{
	int slot = -1;

	if (mType != D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
	{
		LOG(LogCore, Warning, "Trying to allocate an SRV in the wrong heap");
		assert(false && "Trying to allocate an SRV in the wrong heap");
		return DXHeapHandle();
	}
	if (mClearList.size() > 0)
	{
		slot = mClearList[0];
		mClearList.erase(mClearList.begin());
	}
	else if (mResourceCount <= mMaxResources)
	{
		slot = mResourceCount;
		mResourceCount++;
	}
	else
	{
		LOG(LogCore, Fatal, "Descriptor heap maximum reached");
		assert(false && "Descriptor heap maximum reached");
		return DXHeapHandle();
	}

	Engine::Device& engineDevice = Engine::Device::Get();
	ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), slot, mDescriptorSize);
	device->CreateShaderResourceView(resource->Get(), desc, handle);


	return DXHeapHandle(slot, shared_from_this());
}

DXHeapHandle DXDescHeap::AllocateUAV(DXResource* resource, D3D12_UNORDERED_ACCESS_VIEW_DESC* desc)
{
	int slot = -1;

	if (mType != D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
	{
		LOG(LogCore, Warning, "Trying to allocate an SRV in the wrong heap");
		assert(false && "Trying to allocate an SRV in the wrong heap");
		return DXHeapHandle();
	}
	if (mClearList.size() > 0)
	{
		slot = mClearList[0];
		mClearList.erase(mClearList.begin());
	}
	else if (mResourceCount <= mMaxResources)
	{
		slot = mResourceCount;
		mResourceCount++;
	}
	else
	{
		LOG(LogCore, Fatal, "Descriptor heap maximum reached");
		assert(false && "Descriptor heap maximum reached");
		return DXHeapHandle();
	}

	Engine::Device& engineDevice = Engine::Device::Get();
	ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), slot, mDescriptorSize);
	device->CreateUnorderedAccessView(resource->Get(), nullptr, desc, handle);


	return DXHeapHandle(slot, shared_from_this());
}

DXHeapHandle DXDescHeap::AllocateRenderTarget(DXResource* resource, D3D12_RENDER_TARGET_VIEW_DESC* desc)
{
	int slot = -1;

	if (mType != D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
	{
		LOG(LogCore, Warning, "Trying to allocate an RTV in the wrong heap");
		assert(false && "Trying to allocate an RTV in the wrong heap");
		return DXHeapHandle();
	}

	if (mClearList.size() > 0)
	{
		slot = mClearList[0];
		mClearList.erase(mClearList.begin());
	}
	else if (mResourceCount < mMaxResources)
	{
		slot = mResourceCount;
		mResourceCount++;
	}
	else
	{
		LOG(LogCore, Fatal, "Descriptor heap maximum reached");
		assert(false && "Descriptor heap maximum reached");
		return DXHeapHandle();
	}

	Engine::Device& engineDevice = Engine::Device::Get();
	ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());

	CD3DX12_CPU_DESCRIPTOR_HANDLE handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), slot, mDescriptorSize);
	device->CreateRenderTargetView(resource->Get(), desc, handle);

	return DXHeapHandle(slot, shared_from_this());
}

DXHeapHandle DXDescHeap::AllocateRenderTarget(DXResource* resource, ID3D12Device5* device, D3D12_RENDER_TARGET_VIEW_DESC* desc)
{
	int slot = -1;

	if (mType != D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
	{
		LOG(LogCore, Warning, "Trying to allocate an RTV in the wrong heap");
		assert(false && "Trying to allocate an RTV in the wrong heap");
		return DXHeapHandle();
	}

	if (mClearList.size() > 0)
	{
		slot = mClearList[0];
		mClearList.erase(mClearList.begin());
	}
	else if (mResourceCount <= mMaxResources)
	{
		slot = mResourceCount;
		mResourceCount++;
	}
	else
	{
		LOG(LogCore, Fatal, "Descriptor heap maximum reached");
		assert(false && "Descriptor heap maximum reached");
		return DXHeapHandle();
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), slot, mDescriptorSize);
	device->CreateRenderTargetView(resource->Get(), desc, handle);

	return DXHeapHandle(slot, shared_from_this());
}

DXHeapHandle DXDescHeap::AllocateDepthStencil(DXResource* resource, D3D12_DEPTH_STENCIL_VIEW_DESC* desc)
{
	int slot = -1;

	if (mType != D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
	{
		LOG(LogCore, Warning, "Trying to allocate a DSV in the wrong heap");
		assert(false && "Trying to allocate an DSV in the wrong heap");
		return DXHeapHandle();
	}

	if (mClearList.size() > 0)
	{
		slot = mClearList[0];
		mClearList.erase(mClearList.begin());
	}
	else if (mResourceCount <= mMaxResources)
	{
		slot = mResourceCount;
		mResourceCount++;
	}
	else
	{
		LOG(LogCore, Fatal, "Descriptor heap maximum reached");
		assert(false && "Descriptor heap maximum reached");
		return DXHeapHandle();
	}

	Engine::Device& engineDevice = Engine::Device::Get();
	ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());

	CD3DX12_CPU_DESCRIPTOR_HANDLE handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), slot, mDescriptorSize);
	device->CreateDepthStencilView(resource->Get(), desc, handle);

	return DXHeapHandle(slot, shared_from_this());
}

DXHeapHandle DXDescHeap::AllocateDepthStencil(DXResource* resource, ID3D12Device5* device, D3D12_DEPTH_STENCIL_VIEW_DESC* desc)
{
	int slot = -1;

	if (mType != D3D12_DESCRIPTOR_HEAP_TYPE_DSV) {
		LOG(LogCore, Warning, "Trying to allocate a DSV in the wrong heap");
		assert(false && "Trying to allocate an DSV in the wrong heap");
		return DXHeapHandle();
	}

	if (mClearList.size() > 0)
	{
		slot = mClearList[0];
		mClearList.erase(mClearList.begin());
	}
	else if (mResourceCount <= mMaxResources)
	{
		slot = mResourceCount;
		mResourceCount++;
	}
	else
	{
		LOG(LogCore, Fatal, "Descriptor heap maximum reached");
		assert(false && "Descriptor heap maximum reached");
		return DXHeapHandle();
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), slot, mDescriptorSize);
	device->CreateDepthStencilView(resource->Get(), desc, handle);

	return DXHeapHandle(slot, shared_from_this());
}

void DXDescHeap::DeallocateResource(int slot)
{
	for (size_t i = 0; i < mClearList.size(); i++)
	{
		if (mClearList[i] == slot)
			return;
	}

	mClearList.push_back(slot);
}