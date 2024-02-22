#pragma once
#include "DXDefines.h"

class DXResource;
class DXConstBuffer
{
public:
	DXConstBuffer() {};
	DXConstBuffer(const ComPtr<ID3D12Device5>& device, size_t dataSize, int numOfObjects, const char* bufferDebugName, int frameNumber);
	void Update(void* data, size_t dataSize, int offsetIndex, int frameIndex);
	void Bind(const ComPtr<ID3D12GraphicsCommandList4>& command, int rootParameterIndex, int offsetIndex, int frameIndexs);
	void BindToCompute(const ComPtr<ID3D12GraphicsCommandList4>& command, int rootParameterIndex, int offsetIndex, int frameIndexs);

	const size_t GetBufferPerObjectAlignedSize() { return mBufferPerObjectAlignedSize; }
	const size_t GetGPUPointer(int slot, int bufferIndex);  /*{return buffer[bufferIndex]->GetResource()->GetGPUVirtualAddress() + (bufferPerObjectAlignedSize * slot); }*/

private:
		std::vector<std::unique_ptr<DXResource>> mBuffers;
		UINT8* mBufferGPUAddress[2] = { 0,0 };
		size_t mBufferPerObjectAlignedSize = 0;
		size_t mBufferSize = 0;
		bool inScope = false;
};

