#pragma once
#include <vector>
#include <memory>
#pragma warning(push)
#pragma warning(disable: 4005)
#include <wrl.h>
#define NOMINMAX
#include <Windows.h>
#pragma warning(pop) 

class DXSignature;
using namespace Microsoft::WRL;
using uint = unsigned int;

class DXPipeline
{
public:
	DXPipeline() {};
	void CreatePipeline(ComPtr<ID3D12Device5> device, const std::unique_ptr<DXSignature>& root, LPCWSTR name);

	void AddInput(LPCSTR name, DXGI_FORMAT format, const uint slot);
	void SetRasterizer(const CD3DX12_RASTERIZER_DESC& rasterizer);
	void SetBlendState(const CD3DX12_BLEND_DESC& blend);
	void SetDepthState(const CD3DX12_DEPTH_STENCIL_DESC& depth);
	void SetVertexAndPixelShaders(LPVOID vsBuffer, SIZE_T vsSize, LPVOID psBuffer, SIZE_T psSize);
	void SetComputeShader(LPVOID computeShaderBuffer, SIZE_T computeShaderSize);
	void SetMsaaCountAndQuality(uint count, uint quality);
	void AddRenderTarget(DXGI_FORMAT format);

	static ComPtr<ID3DBlob> ShaderToBlob(const char* path, const char* shaderVersion, const char* functionName = nullptr);

	ComPtr<ID3D12PipelineState> GetPipeline() const { return pipeline; }

private:
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputs;
	std::vector<DXGI_FORMAT> renderTargetFormats;

	LPVOID vertexShaderBuffer = nullptr;
	SIZE_T vertexShaderSize = 0;

	LPVOID fragmentShaderBuffer = nullptr;
	SIZE_T fragmentShaderSize = 0;

	LPVOID computeShaderBuffer = nullptr;
	SIZE_T computeShaderSize = 0;

	DXGI_FORMAT depthFormat = DXGI_FORMAT_D32_FLOAT;
	CD3DX12_RASTERIZER_DESC rast = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	CD3DX12_BLEND_DESC blend = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	CD3DX12_DEPTH_STENCIL_DESC depth = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	uint msaaCount = 1;
	uint msaaQuality = 0;
	ComPtr<ID3D12PipelineState> pipeline;
};

