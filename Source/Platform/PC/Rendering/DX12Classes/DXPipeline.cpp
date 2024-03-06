#include "Precomp.h"
#include "../Include/Platform/PC/Rendering/DX12Classes/DxPipeline.h"
#include "../Include/Platform/PC/Rendering/DX12Classes/DXSignature.h"

void DXPipeline::CreatePipeline(ComPtr<ID3D12Device5> device, const DXSignature* root, LPCWSTR name)
{
	HRESULT hr;

	if (computeShaderBuffer == nullptr) {
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout.NumElements = static_cast<UINT>(inputs.size());
		psoDesc.InputLayout.pInputElementDescs = inputs.data();
		psoDesc.pRootSignature = root->GetSignature().Get();
		psoDesc.VS.BytecodeLength = vertexShaderSize;
		psoDesc.VS.pShaderBytecode = vertexShaderBuffer;
		psoDesc.PS.BytecodeLength = fragmentShaderSize;
		psoDesc.PS.pShaderBytecode = fragmentShaderBuffer;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.SampleDesc.Count = msaaCount;
		psoDesc.SampleDesc.Quality = msaaQuality;
		psoDesc.SampleMask = 0xffffffff;
		psoDesc.RasterizerState = rast;
		psoDesc.BlendState = blend;
		if (renderTargetFormats.size() <= 0)
		{
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		}
		else
		{
			psoDesc.NumRenderTargets = static_cast<UINT>(renderTargetFormats.size());
			for (size_t i = 0; i < renderTargetFormats.size(); i++)
				psoDesc.RTVFormats[i] = renderTargetFormats[i];
		}
		psoDesc.DSVFormat = depthFormat;
		psoDesc.DepthStencilState = depth;
		hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeline));
	}
	else {
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = root->GetSignature().Get();
		psoDesc.CS = { reinterpret_cast<UINT8*>(computeShaderBuffer), computeShaderSize };
		hr = device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pipeline));
	}


	if (FAILED(hr)) {
		MessageBox(NULL, L"Failed to create default pipeline", L"FATAL ERROR!", MB_ICONERROR | MB_OK);
		assert(false && "Failed to create default pipeline");
	}
	pipeline->SetName(name);
}

void DXPipeline::AddInput(LPCSTR name, DXGI_FORMAT format, const uint slot)
{
	D3D12_INPUT_ELEMENT_DESC input;
	input = { name, 0, format, slot, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	inputs.push_back(input);
}

void DXPipeline::SetRasterizer(const CD3DX12_RASTERIZER_DESC& rasterizer)
{
	rast = rasterizer;
}

void DXPipeline::SetBlendState(const CD3DX12_BLEND_DESC& blendState)
{
	blend = blendState;
}

void DXPipeline::SetDepthState(const CD3DX12_DEPTH_STENCIL_DESC& depthStencil)
{
	depth = depthStencil;
}

void DXPipeline::SetVertexAndPixelShaders(LPVOID vsBuffer, SIZE_T vsSize, LPVOID psBuffer, SIZE_T psSize)
{
	vertexShaderBuffer = vsBuffer;
	vertexShaderSize = vsSize;
	fragmentShaderBuffer = psBuffer;
	fragmentShaderSize = psSize;
}

void DXPipeline::SetComputeShader(LPVOID computeShaderB, SIZE_T computeShaderS)
{
	computeShaderBuffer = computeShaderB;
	computeShaderSize = computeShaderS;
}

void DXPipeline::SetMsaaCountAndQuality(uint count, uint quality)
{
	msaaCount = count;
	msaaQuality = quality;
}

void DXPipeline::AddRenderTarget(DXGI_FORMAT format)
{
	renderTargetFormats.push_back(format);
}

ComPtr<ID3DBlob> DXPipeline::ShaderToBlob(const char* path, const char* shaderVersion, const char* functionName)
{
	ComPtr<ID3DBlob> shader; // d3d blob for holding vertex shader bytecode
	ComPtr<ID3DBlob> errorBuff;

	wchar_t* wString = new wchar_t[4096];
	MultiByteToWideChar(CP_ACP, 0, path, -1, wString, 4096);
	HRESULT hr;

	if(functionName != nullptr)
		hr = D3DCompileFromFile(wString, nullptr, nullptr, functionName, shaderVersion, D3DCOMPILE_DEBUG, 0, &shader, &errorBuff);
	else
		hr = D3DCompileFromFile(wString, nullptr, nullptr, "main", shaderVersion, D3DCOMPILE_DEBUG, 0, &shader, &errorBuff);

	if (FAILED(hr)) {
		printf((const char*)errorBuff->GetBufferPointer());
		assert(false && "Check output");
	}

	delete[] wString;

	return shader;
}
