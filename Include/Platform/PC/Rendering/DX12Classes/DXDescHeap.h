#pragma once
#include "DXDefines.h"
#include "DXResource.h"


class DXHeapHandle;
class DXDescHeap : public std::enable_shared_from_this<DXDescHeap>
{
	friend DXHeapHandle;
	friend std::shared_ptr<DXDescHeap>;

public:
	static std::shared_ptr<DXDescHeap> Construct(const ComPtr<ID3D12Device5>& device, int numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type, LPCWSTR name, D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE) {
		auto variable =  std::make_shared<DXDescHeap>(device, numDescriptors, type, name, flags);
		return variable;
	}

	DXDescHeap(DXDescHeap&& other) {
		mDescriptorHeap = std::move(other.mDescriptorHeap);
		mDescriptorSize = other.mDescriptorSize; other.mDescriptorSize = 0;
		mType = other.mType; other.mType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
		mMaxResources = other.mMaxResources; other.mMaxResources = 0;
		mResourceCount = other.mResourceCount; other.mResourceCount = 0;
		mClearList = std::move(other.mClearList);
	}

	DXDescHeap& operator=(DXDescHeap&& other) noexcept {
		mDescriptorHeap = std::move(other.mDescriptorHeap);
		mDescriptorSize = other.mDescriptorSize; other.mDescriptorSize = 0;
		mType = other.mType; other.mType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
		mMaxResources = other.mMaxResources; other.mMaxResources = 0;
		mResourceCount = other.mResourceCount; other.mResourceCount = 0;
		mClearList = std::move(other.mClearList);
	}


	//DONOT USE THIS, USE DXHeapHandle::Construct!!!!!!
	DXDescHeap(const ComPtr<ID3D12Device5>& device, int numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type, LPCWSTR name, D3D12_DESCRIPTOR_HEAP_FLAGS flags);

	void BindToGraphics(ComPtr<ID3D12GraphicsCommandList4> commandList, unsigned int rootSlot, const DXHeapHandle& handle);
	void BindToCompute(ComPtr<ID3D12GraphicsCommandList4> commandList, unsigned int rootSlot, const DXHeapHandle& handle);

	void BindRenderTargets(ComPtr<ID3D12GraphicsCommandList4> commandList, const DXHeapHandle* handles, const DXHeapHandle& dsvHandle, unsigned int numRtv =1);
	void BindRenderTargets(ComPtr<ID3D12GraphicsCommandList4> commandList, const DXHeapHandle& rtvHeapSlot);

	void ClearRenderTarget(ComPtr<ID3D12GraphicsCommandList4> commandList, const DXHeapHandle& handle, const float* clearData);
	void ClearDepthStencil(ComPtr<ID3D12GraphicsCommandList4> commandList, const DXHeapHandle& handle);

	DXHeapHandle AllocateResource(DXResource* resource, D3D12_SHADER_RESOURCE_VIEW_DESC* desc);
	DXHeapHandle AllocateRenderTarget(DXResource* resource, D3D12_RENDER_TARGET_VIEW_DESC* desc);
	DXHeapHandle AllocateRenderTarget(DXResource* resource, ID3D12Device5* device, D3D12_RENDER_TARGET_VIEW_DESC* desc);
	DXHeapHandle AllocateDepthStencil(DXResource* resource, D3D12_DEPTH_STENCIL_VIEW_DESC* desc);
	DXHeapHandle AllocateDepthStencil(DXResource* resource, ID3D12Device5* device, D3D12_DEPTH_STENCIL_VIEW_DESC* desc);

	ID3D12DescriptorHeap* Get() const { return mDescriptorHeap.Get(); }
	const int GetDescriptorSize() const { return mDescriptorSize; }

private:
	
	void DeallocateResource(int slot);

	ComPtr<ID3D12DescriptorHeap> mDescriptorHeap;
	int mDescriptorSize;
	D3D12_DESCRIPTOR_HEAP_TYPE mType;

	int mMaxResources;
	int mResourceCount = 0;
	std::vector<int> mClearList;


};

