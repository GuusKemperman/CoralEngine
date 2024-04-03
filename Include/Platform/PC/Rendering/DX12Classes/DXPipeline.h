#pragma once
#include <vector>
#include <memory>
#include "Platform/PC/Rendering/DX12Classes/DXDefines.h"

struct ID3D12Device5;

class DXPipelineBuilder
{
public:
	DXPipelineBuilder() {};

	DXPipelineBuilder& AddInput(LPCSTR name, DXGI_FORMAT format, const uint32 slot);
	DXPipelineBuilder& SetRasterizer(const CD3DX12_RASTERIZER_DESC& rasterizer);
	DXPipelineBuilder& SetBlendState(const CD3DX12_BLEND_DESC& blend);
	DXPipelineBuilder& SetDepthState(const CD3DX12_DEPTH_STENCIL_DESC& depth);
	DXPipelineBuilder& SetVertexAndPixelShaders(LPVOID vsBuffer, SIZE_T vsSize, LPVOID psBuffer, SIZE_T psSize);
	DXPipelineBuilder& SetComputeShader(LPVOID computeShaderBuffer, SIZE_T computeShaderSize);
	DXPipelineBuilder& SetMsaaCountAndQuality(uint32 count, uint32 quality);
	DXPipelineBuilder& AddRenderTarget(DXGI_FORMAT format);
	DXPipelineBuilder& SetPrimitiveTopology(const D3D12_PRIMITIVE_TOPOLOGY_TYPE& topology);

	ComPtr<ID3D12PipelineState> Build(ComPtr<ID3D12Device5> device, const ComPtr<ID3D12RootSignature>& root, LPCWSTR name) const;

	static ComPtr<ID3DBlob> ShaderToBlob(const char* path, const char* shaderVersion, const char* functionName = nullptr);

private:
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputs;
	std::vector<DXGI_FORMAT> mRenderTargetFormats;

	LPVOID mVertexShaderBuffer = nullptr;
	SIZE_T mVertexShaderSize = 0;

	LPVOID mFragmentShaderBuffer = nullptr;
	SIZE_T mFragmentShaderSize = 0;

	LPVOID mComputeShaderBuffer = nullptr;
	SIZE_T mComputeShaderSize = 0;

	DXGI_FORMAT mDepthFormat = DXGI_FORMAT_D32_FLOAT;
	D3D12_PRIMITIVE_TOPOLOGY_TYPE mTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	CD3DX12_RASTERIZER_DESC mRast = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	CD3DX12_BLEND_DESC mBlend = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	CD3DX12_DEPTH_STENCIL_DESC mDepth = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	uint32 mMsaaCount = 1;
	uint32 mMsaaQuality = 0;
};

