#include "Precomp.h"
#include "../Include/Platform/PC/Rendering/DX12Classes/DXSignature.h"

ComPtr<ID3D12RootSignature> DXSignatureBuilder::Build(ComPtr<ID3D12Device5> device, LPCWSTR name) const
{
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(static_cast<UINT>(mParameters.size()),
		mParameters.data(),
		static_cast<UINT>(mSamplers.size()),
		mSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

	ComPtr<ID3DBlob> serializedSignature;
	ComPtr<ID3DBlob> errBlob;
	HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedSignature, &errBlob);

	if (FAILED(hr)) {
		printf((const char*)errBlob->GetBufferPointer());
		assert(false && "Failed to serialize root signature");
	}

	ComPtr<ID3D12RootSignature> signature;
	hr = device->CreateRootSignature(0, serializedSignature->GetBufferPointer(), serializedSignature->GetBufferSize(), IID_PPV_ARGS(&signature));
	if (FAILED(hr)) {
		MessageBox(NULL, L"Failed to initialize root signature", L"FATAL ERROR!", MB_ICONERROR | MB_OK);
		assert(false && "Failed to initialize root signature");
	}
	signature->SetName(name);
	return signature;
}

DXSignatureBuilder& DXSignatureBuilder::Add32BitConstant(const uint32 shaderRegister, D3D12_SHADER_VISIBILITY shader, int num32BitValues)
{
	D3D12_ROOT_DESCRIPTOR desc;
	desc.RegisterSpace = 0;
	desc.ShaderRegister = shaderRegister;

	D3D12_ROOT_PARAMETER par = {};
	par.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	par.Constants.Num32BitValues = num32BitValues;
	par.Constants.RegisterSpace = 0;
	par.Constants.ShaderRegister = shaderRegister;
	par.ShaderVisibility = shader;

	mParameters.push_back(par);
	return *this;
}

DXSignatureBuilder& DXSignatureBuilder::AddCBuffer(const uint32 shaderRegister, D3D12_SHADER_VISIBILITY shader)
{
	D3D12_ROOT_DESCRIPTOR desc;
	desc.RegisterSpace = 0;
	desc.ShaderRegister = shaderRegister;

	D3D12_ROOT_PARAMETER par;
	par.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	par.Descriptor = desc;
	par.ShaderVisibility = shader;

	mParameters.push_back(par);
	return *this;
}

DXSignatureBuilder& DXSignatureBuilder::AddTable(D3D12_SHADER_VISIBILITY shader, D3D12_DESCRIPTOR_RANGE_TYPE rangeType, int numDescriptors, int shaderRegister)
{
	D3D12_DESCRIPTOR_RANGE range;
	range.RangeType = rangeType;
	range.NumDescriptors = numDescriptors;
	range.BaseShaderRegister = shaderRegister;
	range.RegisterSpace = 0;
	range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	mRanges[mRangeCounter] = range;

	D3D12_ROOT_DESCRIPTOR_TABLE descriptorTable;
	descriptorTable.NumDescriptorRanges = 1;
	descriptorTable.pDescriptorRanges = &mRanges[mRangeCounter];
	mRangeCounter++;

	D3D12_ROOT_PARAMETER par;
	par.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	par.DescriptorTable = descriptorTable;
	par.ShaderVisibility = shader;
	mParameters.push_back(par);
	return *this;
}

DXSignatureBuilder& DXSignatureBuilder::AddSampler(const uint32 shaderRegister, D3D12_SHADER_VISIBILITY shader, D3D12_TEXTURE_ADDRESS_MODE mode, D3D12_FILTER filter)
{
	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = filter;
	sampler.AddressU = mode;
	sampler.AddressV = mode;
	sampler.AddressW = mode;
	sampler.MipLODBias = 0;
	sampler.MaxAnisotropy = 0;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister = shaderRegister;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = shader;

	mSamplers.push_back(sampler);
	return *this;
}
