#pragma once
#pragma warning(push)
#pragma warning(disable: 4005)
#include <wrl.h>
#define NOMINMAX
#include <Windows.h>
#pragma warning(pop) 

#include "DXDefines.h"

#include <memory>
#include <vector>


using namespace Microsoft::WRL;
class DXResource
{
public:
	DXResource(){};
	DXResource(const ComPtr<ID3D12Device5>& device, const CD3DX12_HEAP_PROPERTIES& heapProperties, const CD3DX12_RESOURCE_DESC& desc, D3D12_CLEAR_VALUE* clearValue, const char* name);
	DXResource(ComPtr<ID3D12Resource> res, D3D12_RESOURCE_STATES resState);

	ComPtr<ID3D12Resource> GetResource() { return resource; }
	ComPtr<ID3D12Resource> GetUploadResource(int subresource) { return uploadBuffers[subresource]->GetResource(); }
	ID3D12Resource* Get() { return resource.Get(); }
	void SetResource(ComPtr<ID3D12Resource> res) { resource = res; }

	CD3DX12_RESOURCE_DESC GetDesc() const { return desc; }

	void ChangeState(const ComPtr<ID3D12GraphicsCommandList>& list, D3D12_RESOURCE_STATES dstState);
	void CreateUploadBuffer(const ComPtr<ID3D12Device5>& device, int dataSize, int currentSubresource);
	void Update(const ComPtr<ID3D12GraphicsCommandList>& list, D3D12_SUBRESOURCE_DATA data, D3D12_RESOURCE_STATES dstState, int currentSubresource, int totalSubresources);
	bool updateBuffer = false;
	bool uploadBuffer = false;

private:
	D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
	CD3DX12_RESOURCE_DESC desc {};
	ComPtr<ID3D12Resource> resource;
	std::vector<std::unique_ptr<DXResource>> uploadBuffers;
};

