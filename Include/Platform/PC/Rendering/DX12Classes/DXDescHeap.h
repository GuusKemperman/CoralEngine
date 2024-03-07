#pragma once
#include "DXDefines.h"


class DXDescHeap
{
public:
	DXDescHeap(const ComPtr<ID3D12Device5>& device, int numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type, LPCWSTR name, D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	void BindToGraphics(ComPtr<ID3D12GraphicsCommandList4> commandList, unsigned int rootSlot, unsigned int heapSlot);
	void BindToCompute(ComPtr<ID3D12GraphicsCommandList4> commandList, unsigned int rootSlot, unsigned int heapSlot);

	void BindRenderTargets(ComPtr<ID3D12GraphicsCommandList4> commandList, unsigned int* rtvHeapSlots, const CD3DX12_CPU_DESCRIPTOR_HANDLE& dsvHandle, unsigned int numRtv = 1);
	void BindRenderTargets(ComPtr<ID3D12GraphicsCommandList4> commandList, unsigned int rtvHeapSlot);

	void ClearRenderTarget(ComPtr<ID3D12GraphicsCommandList4> commandList, unsigned int heapSlot, const float* clearData);
	void ClearDepthStencil(ComPtr<ID3D12GraphicsCommandList4> commandList, unsigned int heapSlot);

	ID3D12DescriptorHeap* Get() const { return descriptorHeap.Get(); }
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(unsigned int heapSlot) const;
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(unsigned int heapSlot) const;
	const int GetDescriptorSize() const { return descriptorSize; }

private:
	ComPtr<ID3D12DescriptorHeap> descriptorHeap;
	int descriptorSize;
	D3D12_DESCRIPTOR_HEAP_TYPE type;


};

