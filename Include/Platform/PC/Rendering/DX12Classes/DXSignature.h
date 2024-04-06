#pragma once
#include <vector>
#include "DXDefines.h"

class DXSignatureBuilder
{
public:
	DXSignatureBuilder(int numberOfTables) { mRanges.resize(numberOfTables); };

	DXSignatureBuilder& Add32BitConstant(const uint32 shaderRegister, D3D12_SHADER_VISIBILITY shader, int num32BitValues);
	DXSignatureBuilder& AddCBuffer(const uint32 shaderRegister, D3D12_SHADER_VISIBILITY shader);
	DXSignatureBuilder& AddTable(D3D12_SHADER_VISIBILITY shader, D3D12_DESCRIPTOR_RANGE_TYPE rangeType, int numDescriptors, int shaderRegister);
	DXSignatureBuilder& AddSampler(const uint32 shaderRegister, D3D12_SHADER_VISIBILITY shader, D3D12_TEXTURE_ADDRESS_MODE mode, D3D12_FILTER filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT);

	ComPtr<ID3D12RootSignature> Build(ComPtr<ID3D12Device5> device, LPCWSTR name) const;

private:
	int mRangeCounter = 0;
	std::vector<D3D12_DESCRIPTOR_RANGE> mRanges;
	std::vector<D3D12_ROOT_PARAMETER> mParameters;
	std::vector<D3D12_STATIC_SAMPLER_DESC> mSamplers;
};

