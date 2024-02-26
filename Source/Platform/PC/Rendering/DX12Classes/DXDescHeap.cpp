#include "Precomp.h"
#include "../Include/Platform/PC/Rendering/DXDescHeap.h"

DXDescHeap::DXDescHeap(const ComPtr<ID3D12Device5>& device, int numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type, LPCWSTR name, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
{
	D3D12_DESCRIPTOR_HEAP_DESC renderTargetDesc = {};
	renderTargetDesc.NumDescriptors = numDescriptors;
	renderTargetDesc.Type = type;
	renderTargetDesc.Flags = flags;

	this->type = type;

	HRESULT hr = device->CreateDescriptorHeap(&renderTargetDesc, IID_PPV_ARGS(&descriptorHeap));
	if (FAILED(hr)) {
		LOG(LogCore, Fatal, "Failed to create descriptor heap");
		assert(false && "Failed to create descriptor heap");
	}
	descriptorSize = device->GetDescriptorHandleIncrementSize(type);
	descriptorHeap->SetName(name);
}

void DXDescHeap::BindToGraphics(ComPtr<ID3D12GraphicsCommandList4> commandList, unsigned int rootSlot, unsigned int heapSlot)
{
	if (heapSlot < 0)
		return;

	CD3DX12_GPU_DESCRIPTOR_HANDLE handle(descriptorHeap->GetGPUDescriptorHandleForHeapStart(), heapSlot, descriptorSize);
	commandList->SetGraphicsRootDescriptorTable(rootSlot, handle);
}

void DXDescHeap::BindToCompute(ComPtr<ID3D12GraphicsCommandList4> commandList, unsigned int rootSlot, unsigned int heapSlot)
{
	if (heapSlot < 0)
		return;

	CD3DX12_GPU_DESCRIPTOR_HANDLE handle(descriptorHeap->GetGPUDescriptorHandleForHeapStart(), heapSlot, descriptorSize);
	commandList->SetComputeRootDescriptorTable(rootSlot, handle);
}

void DXDescHeap::BindRenderTargets(ComPtr<ID3D12GraphicsCommandList4> commandList, unsigned int* rtvHeapSlots, const CD3DX12_CPU_DESCRIPTOR_HANDLE& dsvHandle, unsigned int numRtv)
{
	std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> rtvHandles(numRtv);

	for (unsigned int i = 0; i < numRtv; i++)
		rtvHandles[i] = GetCPUHandle(rtvHeapSlots[i]);

	commandList->OMSetRenderTargets(static_cast<UINT>(rtvHandles.size()), rtvHandles.data(), FALSE, &dsvHandle);
}

void DXDescHeap::BindRenderTargets(ComPtr<ID3D12GraphicsCommandList4> commandList, unsigned int rtvHeapSlot) {
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle = GetCPUHandle(rtvHeapSlot);
	commandList->OMSetRenderTargets(1, &cpuHandle, FALSE, nullptr);
}


void DXDescHeap::ClearRenderTarget(ComPtr<ID3D12GraphicsCommandList4> commandList, unsigned int heapSlot, const float* clearData)
{
	if (type != D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
		return;

	commandList->ClearRenderTargetView(GetCPUHandle(heapSlot), clearData, 0, nullptr);
}

void DXDescHeap::ClearDepthStencil(ComPtr<ID3D12GraphicsCommandList4> commandList, unsigned int heapSlot)
{
	if (type != D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
		return;

	commandList->ClearDepthStencilView(GetCPUHandle(heapSlot), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DXDescHeap::GetCPUHandle(unsigned int heapSlot) const
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(descriptorHeap->GetCPUDescriptorHandleForHeapStart(), heapSlot, descriptorSize);

	return handle;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE DXDescHeap::GetGPUHandle(unsigned int heapSlot) const
{
	CD3DX12_GPU_DESCRIPTOR_HANDLE handle(descriptorHeap->GetGPUDescriptorHandleForHeapStart(), heapSlot, descriptorSize);

	return handle;
}
