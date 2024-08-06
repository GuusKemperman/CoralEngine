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
		IID_PPV_ARGS(&mResource)
	);
	mState = state;
	mDesc = descr;

	ASSERT_LOG(!(FAILED(hr)), "Resource creation failed");

	wchar_t wString[4096];
	MultiByteToWideChar(CP_ACP, 0, name, -1, wString, 4096);
	mResource->SetName(wString);
}

DXResource::DXResource(ComPtr<ID3D12Resource> res, D3D12_RESOURCE_STATES resState)
{
	mResource = res;
	mState = resState;
}

DXResource::~DXResource()
{
	for (size_t i = 0; i < mUploadBuffers.size(); i++) {
		mUploadBuffers[i] = nullptr;
	}

	CE::Device& engineDevice = CE::Device::Get();
	engineDevice.AddToDeallocation(std::move(mResource));
}

void DXResource::ChangeState(const ComPtr<ID3D12GraphicsCommandList>& list, D3D12_RESOURCE_STATES dstState)
{
	if (dstState == mState)
		return;

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(mResource.Get(), mState, dstState);
	list->ResourceBarrier(1, &barrier);
	mState = dstState;
}

void DXResource::CreateUploadBuffer(const ComPtr<ID3D12Device5>& device, int dataSize, int currentSubresource)
{
	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(dataSize);
	if (static_cast<int>(mUploadBuffers.size()) <= currentSubresource)
	{
		mUploadBuffers.resize(currentSubresource + 1);
	}

	mUploadBuffers[currentSubresource] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "Upload buffer", D3D12_RESOURCE_STATE_GENERIC_READ);
}

void DXResource::Update(const ComPtr<ID3D12GraphicsCommandList>& list, D3D12_SUBRESOURCE_DATA data, D3D12_RESOURCE_STATES dstState, int currentSubresource, int totalSubresources)
{
	ChangeState(list, D3D12_RESOURCE_STATE_COPY_DEST);
	UpdateSubresources(list.Get(), mResource.Get(), mUploadBuffers[currentSubresource]->mResource.Get(), 0, currentSubresource, totalSubresources, &data);
	ChangeState(list, dstState);
}
