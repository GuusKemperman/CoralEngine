#pragma once
#include "DXDefines.h"

#include <memory>
#include <vector>
class DXResource
{
public:
	DXResource(){};
	DXResource(const ComPtr<ID3D12Device5>& device, const CD3DX12_HEAP_PROPERTIES& heapProperties, const CD3DX12_RESOURCE_DESC& descr, D3D12_CLEAR_VALUE* clearValue, const char* name, D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON);
	DXResource(ComPtr<ID3D12Resource> res, D3D12_RESOURCE_STATES resState);
	~DXResource();
	
	ComPtr<ID3D12Resource> GetResource() { return mResource; }
	ComPtr<ID3D12Resource> GetUploadResource(int subresource) { return mUploadBuffers[subresource]->GetResource(); }
	ID3D12Resource* Get() { return mResource.Get(); }
	void SetResource(ComPtr<ID3D12Resource> res) { mResource = res; }

	CD3DX12_RESOURCE_DESC GetDesc() const { return mDesc; }
	D3D12_RESOURCE_STATES GetState() const { return mState; }

	void ChangeState(const ComPtr<ID3D12GraphicsCommandList>& list, D3D12_RESOURCE_STATES dstState);
	void CreateUploadBuffer(const ComPtr<ID3D12Device5>& device, int dataSize, int currentSubresource);
	void Update(const ComPtr<ID3D12GraphicsCommandList>& list, D3D12_SUBRESOURCE_DATA data, D3D12_RESOURCE_STATES dstState, int currentSubresource, int totalSubresources);
	bool mResizeBuffer = false;

private:
	D3D12_RESOURCE_STATES mState = D3D12_RESOURCE_STATE_COMMON;
	CD3DX12_RESOURCE_DESC mDesc {};
	ComPtr<ID3D12Resource> mResource;
	std::vector<std::unique_ptr<DXResource>> mUploadBuffers;
};

