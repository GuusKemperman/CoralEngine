#include "Precomp.h"
#include "../Include/Platform/PC/Rendering/DX12Classes/DxPipeline.h"
#include "../Include/Platform/PC/Rendering/DX12Classes/DXSignature.h"

#ifdef _MSC_VER
#pragma comment(lib, "d3dcompiler") // Automatically link with d3dcompiler.lib as we are using D3DCompile() below.
#endif

ComPtr<ID3D12PipelineState> DXPipelineBuilder::Build(ComPtr<ID3D12Device5> device, const ComPtr<ID3D12RootSignature>& root, LPCWSTR name) const
{
	HRESULT hr;
	ComPtr<ID3D12PipelineState> pipeline;

	if (mComputeShaderBuffer == nullptr) {
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout.NumElements = static_cast<UINT>(mInputs.size());
		psoDesc.InputLayout.pInputElementDescs = mInputs.data();
		psoDesc.pRootSignature = root.Get();
		psoDesc.VS.BytecodeLength = mVertexShaderSize;
		psoDesc.VS.pShaderBytecode = mVertexShaderBuffer;
		psoDesc.PS.BytecodeLength = mFragmentShaderSize;
		psoDesc.PS.pShaderBytecode = mFragmentShaderBuffer;
		psoDesc.PrimitiveTopologyType = mTopology;
		psoDesc.SampleDesc.Count = mMsaaCount;
		psoDesc.SampleDesc.Quality = mMsaaQuality;
		psoDesc.SampleMask = 0xffffffff;
		psoDesc.RasterizerState = mRast;
		psoDesc.BlendState = mBlend;
		if (mRenderTargetFormats.size() <= 0)
		{
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		}
		else
		{
			psoDesc.NumRenderTargets = static_cast<UINT>(mRenderTargetFormats.size());
			for (size_t i = 0; i < mRenderTargetFormats.size(); i++)
				psoDesc.RTVFormats[i] = mRenderTargetFormats[i];
		}
		psoDesc.DSVFormat = mDepthFormat;
		psoDesc.DepthStencilState = mDepth;
		hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeline));
	}
	else {
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = root.Get();
		psoDesc.CS = { reinterpret_cast<UINT8*>(mComputeShaderBuffer), mComputeShaderSize };
		hr = device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pipeline));
	}

	if (FAILED(hr))
	{
		MessageBox(NULL, L"Failed to create pipeline", L"FATAL ERROR!", MB_ICONERROR | MB_OK);
		assert(false && "Failed to create pipeline");
	}
	pipeline->SetName(name);

	return pipeline;
}

DXPipelineBuilder& DXPipelineBuilder::AddInput(LPCSTR name, DXGI_FORMAT format, const uint32 slot)
{
	D3D12_INPUT_ELEMENT_DESC input;
	input = { name, 0, format, slot, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	mInputs.push_back(input);
	return *this;
}

DXPipelineBuilder& DXPipelineBuilder::SetRasterizer(const CD3DX12_RASTERIZER_DESC& rasterizer)
{
	mRast = rasterizer;
	return *this;
}

DXPipelineBuilder& DXPipelineBuilder::SetBlendState(const CD3DX12_BLEND_DESC& blendState)
{
	mBlend = blendState;
	return *this;
}

DXPipelineBuilder& DXPipelineBuilder::SetDepthState(const CD3DX12_DEPTH_STENCIL_DESC& depthStencil)
{
	mDepth = depthStencil;
	return *this;
}

DXPipelineBuilder& DXPipelineBuilder::SetDepthFormat(const DXGI_FORMAT& format)
{
	mDepthFormat = format;
	return *this;
}

DXPipelineBuilder& DXPipelineBuilder::SetVertexAndPixelShaders(LPVOID vsBuffer, SIZE_T vsSize, LPVOID psBuffer, SIZE_T psSize)
{
	mVertexShaderBuffer = vsBuffer;
	mVertexShaderSize = vsSize;
	mFragmentShaderBuffer = psBuffer;
	mFragmentShaderSize = psSize;
	return *this;
}

DXPipelineBuilder& DXPipelineBuilder::SetComputeShader(LPVOID computeShaderB, SIZE_T computeShaderS)
{
	mComputeShaderBuffer = computeShaderB;
	mComputeShaderSize = computeShaderS;
	return *this;
}

DXPipelineBuilder& DXPipelineBuilder::SetMsaaCountAndQuality(uint32 count, uint32 quality)
{
	mMsaaCount = count;
	mMsaaQuality = quality;
	return *this;
}

DXPipelineBuilder& DXPipelineBuilder::AddRenderTarget(DXGI_FORMAT format)
{
	mRenderTargetFormats.push_back(format);
	return *this;
}

DXPipelineBuilder& DXPipelineBuilder::SetPrimitiveTopology(const D3D12_PRIMITIVE_TOPOLOGY_TYPE& topology)
{
	mTopology = topology;
	return *this;
}

ComPtr<ID3DBlob> DXPipelineBuilder::ShaderToBlob(const char* path, const char* shaderVersion, const char* functionName)
{
    ComPtr<ID3DBlob> shader; // d3d blob for holding vertex shader bytecode
    ComPtr<ID3DBlob> errorBuff;
    
    wchar_t* wString = new wchar_t[4096];
    MultiByteToWideChar(CP_ACP, 0, path, -1, wString, 4096);
    HRESULT hr;

	if(functionName != nullptr)
		hr = D3DCompileFromFile(wString, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, functionName, shaderVersion, D3DCOMPILE_DEBUG, 0, &shader, &errorBuff);
	else
		hr = D3DCompileFromFile(wString, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", shaderVersion, D3DCOMPILE_DEBUG, 0, &shader, &errorBuff);

    if (FAILED(hr))
    {
        LOG(LogAssets, Fatal, "Error while compiling HLSL shader: {}", (const char*)errorBuff->GetBufferPointer());
    }

    delete[] wString;

    return shader;
}
