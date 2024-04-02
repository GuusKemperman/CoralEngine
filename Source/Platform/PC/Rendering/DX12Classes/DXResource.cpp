#include "Precomp.h"
#include "Platform/PC/Rendering/DX12Classes/DXResource.h"
#include "Core/Device.h"

DXResource::DXResource(const ComPtr<ID3D12Device5>& device, const CD3DX12_HEAP_PROPERTIES& heapProperties, const CD3DX12_RESOURCE_DESC& descr, D3D12_CLEAR_VALUE* clearValue, const char* name, D3D12_RESOURCE_STATES state)
{
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&descr,
		state,
		clearValue,
		IID_PPV_ARGS(&resource)
	);
	state = state;
	desc = descr;

	if (FAILED(hr)) assert(false && "Resource creation failed");

	wchar_t* wString = new wchar_t[4096];
	MultiByteToWideChar(CP_ACP, 0, name, -1, wString, 4096);
	resource->SetName(wString);
	delete[] wString;
}

DXResource::DXResource(ComPtr<ID3D12Resource> res, D3D12_RESOURCE_STATES resState)
{
	resource = res;
	state = resState;
}

DXResource::~DXResource()
{
	for (size_t i = 0; i < uploadBuffers.size(); i++) {
		uploadBuffers[i] = nullptr;
	}

	CE::Device& engineDevice = CE::Device::Get();
	engineDevice.AddToDeallocation(std::move(resource));
}

void DXResource::ChangeState(const ComPtr<ID3D12GraphicsCommandList>& list, D3D12_RESOURCE_STATES dstState)
{
	if (dstState == state)
		return;

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), state, dstState);
	list->ResourceBarrier(1, &barrier);
	state = dstState;
}

void DXResource::CreateUploadBuffer(const ComPtr<ID3D12Device5>& device, int dataSize, int currentSubresource)
{
	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(dataSize);
	if (uploadBuffers.size() <= currentSubresource)
		uploadBuffers.resize(currentSubresource + 1);

	uploadBuffers[currentSubresource] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "Upload buffer", D3D12_RESOURCE_STATE_GENERIC_READ);
}

void DXResource::Update(const ComPtr<ID3D12GraphicsCommandList>& list, D3D12_SUBRESOURCE_DATA data, D3D12_RESOURCE_STATES dstState, int currentSubresource, int totalSubresources)
{
	ChangeState(list, D3D12_RESOURCE_STATE_COPY_DEST);
	UpdateSubresources(list.Get(), resource.Get(), uploadBuffers[currentSubresource]->resource.Get(), 0, currentSubresource, totalSubresources, &data);
	ChangeState(list, dstState);
}
