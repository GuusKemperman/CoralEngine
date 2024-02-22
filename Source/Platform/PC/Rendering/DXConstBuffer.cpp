#include "Precomp.h"
#include "../Include/Platform/PC/Rendering/DXConstBuffer.h"
#include "../Include/Platform/PC/Rendering/DXResource.h"

DXConstBuffer::DXConstBuffer(const ComPtr<ID3D12Device5>& device, size_t dataSize, int numberOfObjects, const char* bufferDebugName, int frameNumber)
{
	mBufferPerObjectAlignedSize = (dataSize + 255) & ~255;
	mBufferSize = mBufferPerObjectAlignedSize * numberOfObjects;

	mBuffers.resize(frameNumber);

	for (int i = 0; i < frameNumber; i++) {
		auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(static_cast<UINT64>(mBufferSize));

		mBuffers[i] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, bufferDebugName);

		CD3DX12_RANGE readRange(0, 0);
		HRESULT hr = mBuffers[i]->GetResource()->Map(0, &readRange, reinterpret_cast<void**>(&mBufferGPUAddress[i]));
		if (FAILED(hr))
			assert(false && "Buffer mapping failed");
	}
}

void DXConstBuffer::Update(void* data, size_t dataSize, int offsetIndex, int frameIndex)
{
	memcpy(mBufferGPUAddress[frameIndex] + (mBufferPerObjectAlignedSize * offsetIndex), data, dataSize);
}

void DXConstBuffer::Bind(const ComPtr<ID3D12GraphicsCommandList4>& commandList, int rootParameterIndex, int offsetIndex, int frameIndex)
{
	commandList->SetGraphicsRootConstantBufferView(rootParameterIndex, mBuffers[frameIndex]->GetResource()->GetGPUVirtualAddress() + (mBufferPerObjectAlignedSize * offsetIndex));
}

void DXConstBuffer::BindToCompute(const ComPtr<ID3D12GraphicsCommandList4>& command, int rootParameterIndex, int offsetIndex, int frameIndex)
{
	command->SetComputeRootConstantBufferView(rootParameterIndex, mBuffers[frameIndex]->GetResource()->GetGPUVirtualAddress() + (mBufferPerObjectAlignedSize * offsetIndex));
}

const size_t DXConstBuffer::GetGPUPointer(int slot, int bufferIndex)
{
	return mBuffers[bufferIndex]->GetResource()->GetGPUVirtualAddress() + (mBufferPerObjectAlignedSize * slot);
}
