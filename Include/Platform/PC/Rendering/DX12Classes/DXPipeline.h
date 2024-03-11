#pragma once
#include <vector>
#include <memory>
#include "Platform/PC/Rendering/DX12Classes/DXDefines.h"
#include <dxcapi.h>

class DXSignature;
struct ID3D12Device5;
using uint = unsigned int;

class DXPipeline
{
public:
	DXPipeline() {};
	void CreatePipeline(ComPtr<ID3D12Device5> device, const DXSignature* root, LPCWSTR name);

	void AddInput(LPCSTR name, DXGI_FORMAT format, const uint slot);
	void SetRasterizer(const CD3DX12_RASTERIZER_DESC& rasterizer);
	void SetBlendState(const CD3DX12_BLEND_DESC& blend);
	void SetDepthState(const CD3DX12_DEPTH_STENCIL_DESC& depth);
	void SetVertexAndPixelShaders(LPVOID vsBuffer, SIZE_T vsSize, LPVOID psBuffer, SIZE_T psSize);
	void SetComputeShader(LPVOID computeShaderBuffer, SIZE_T computeShaderSize);
	void SetMsaaCountAndQuality(uint count, uint quality);
	void AddRenderTarget(DXGI_FORMAT format);
	void SetPrimitiveTopology(const D3D12_PRIMITIVE_TOPOLOGY_TYPE& topology);

	static ComPtr<ID3DBlob> ShaderToBlob(const char* path, const char* shaderVersion, bool library = false, const char* functionName = nullptr);

	ComPtr<ID3D12PipelineState> GetPipeline() const { return mPipeline; }

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
	uint mMsaaCount = 1;
	uint mMsaaQuality = 0;
	ComPtr<ID3D12PipelineState> mPipeline;
};

