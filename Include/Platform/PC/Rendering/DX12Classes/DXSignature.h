#pragma once
#include <vector>
#include "DXDefines.h"
using uint = unsigned int;

class DXSignature
{
public:
	DXSignature(int numberOfTables) { ranges.resize(numberOfTables); };
	void CreateSignature(ComPtr<ID3D12Device5> device, LPCWSTR name);

	void Add32BitConstant(const uint shaderRegister, D3D12_SHADER_VISIBILITY shader, int num32BitValues);
	void AddCBuffer(const uint shaderRegister, D3D12_SHADER_VISIBILITY shader);
	void AddTable(D3D12_SHADER_VISIBILITY shader, D3D12_DESCRIPTOR_RANGE_TYPE rangeType, int numDescriptors, int shaderRegister);
	void AddSampler(const uint shaderRegister, D3D12_SHADER_VISIBILITY shader, D3D12_TEXTURE_ADDRESS_MODE mode);

	ComPtr<ID3D12RootSignature> GetSignature() const { return signature; }

private:
	int rangeCounter = 0;
	std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
	std::vector<D3D12_ROOT_PARAMETER> parameters;
	std::vector<D3D12_STATIC_SAMPLER_DESC> samplers;
	ComPtr<ID3D12RootSignature> signature;

};

