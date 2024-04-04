#include "Precomp.h"
#include "Rendering/DebugRenderer.h"
#include "Core/FileIO.h"
#include "Core/Device.h"
#include "Platform/PC/Rendering/DX12Classes/DXDefines.h"
#include "Platform/PC/Rendering/DX12Classes/DXResource.h"
#include "Platform/PC/Rendering/DX12Classes/DXPipeline.h"
#include "Platform/PC/Rendering/DX12Classes/DXConstBuffer.h"

#include "World/World.h"
#include "World/Registry.h"
#include "Components/TransformComponent.h"
#include "Components/CameraComponent.h"
#include "Rendering/GPUWorld.h"

class CE::DebugRenderer::Impl
{
public:
	Impl();
	~Impl() = default;
    bool AddLine(const World& world, const glm::vec3& from, const glm::vec3& to, const glm::vec4& color);
    void Render(GPUWorld& gpuWorld);

	ComPtr<ID3D12PipelineState> mDebugPipeline;
};

CE::DebugRenderer::DebugRenderer()
{
	mImpl = std::make_unique<Impl>();
}

CE::DebugRenderer::~DebugRenderer() = default;

CE::DebugRenderer::Impl::Impl()
{
	Device& engineDevice = Device::Get();
	FileIO& fileIO = FileIO::Get();
	ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());

	std::string shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/DebugVertex.hlsl");
	ComPtr<ID3DBlob> v = DXPipelineBuilder::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
	shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/DebugPixel.hlsl");
	ComPtr<ID3DBlob> p = DXPipelineBuilder::ShaderToBlob(shaderPath.c_str(), "ps_5_0", "main");
	mDebugPipeline = DXPipelineBuilder()
		.AddInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0)
		.AddInput("COLOR", DXGI_FORMAT_R32G32B32A32_FLOAT, 1)
		.SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), p->GetBufferPointer(), p->GetBufferSize())
		.SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE)
		.Build(device, reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetSignature()), L"Debug line pipeline");
}

void CE::DebugRenderer::AddLine(const World& world, DebugCategory::Enum category, const glm::vec3& from, const glm::vec3& to, const glm::vec4& color) const
{
	if ((sDebugCategoryFlags & category) != 0)
	{
		mImpl->AddLine(world, from, to, color);
	}
}

void CE::DebugRenderer::Render(const World& world)
{
    mImpl->Render(world.GetGPUWorld());
}

bool CE::DebugRenderer::Impl::AddLine(const World& world, const glm::vec3& from, const glm::vec3& to, const glm::vec4& color)
{
	GPUWorld& gpuWorld = world.GetGPUWorld();
	DebugRenderingData& data = gpuWorld.GetDebugRenderingData();

	if (data.mLineCount < MAX_LINES) 
	{
		uint32 vertexCount = data.mLineCount * 2;

		data.mPositions[vertexCount] = from;
		data.mPositions[vertexCount + 1] = to;
		data.mColors[vertexCount] = color;
		data.mColors[vertexCount + 1] = color;

		data.mLineCount++;
		return true;
	}
	
	LOG(LogCore, Warning, "Trying to render more debug lines than the max amount of {}", MAX_LINES);
    return false;
}

void CE::DebugRenderer::Impl::Render(GPUWorld& gpuWorld)
{
	DebugRenderingData& data = gpuWorld.GetDebugRenderingData();

	if (data.mLineCount == 0) 
	{
		return;
	}

	Device& engineDevice = Device::Get();
	ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
	ID3D12GraphicsCommandList4* uploadCmdList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetUploadCommandList());
	std::shared_ptr<DXDescHeap> resourceHeap = engineDevice.GetDescriptorHeap(RESOURCE_HEAP);
	
	int frameIndex = engineDevice.GetFrameIndex();

	// First update the vertices on GPU
	engineDevice.StartUploadCommands();
	uint32 vertexCount = data.mLineCount * 2;

	D3D12_SUBRESOURCE_DATA positionData{};
	positionData.pData = data.mPositions.data();
	positionData.RowPitch = sizeof(glm::vec3);
	positionData.SlicePitch = sizeof(glm::vec3) * vertexCount;

	data.mVertexPositionBuffer->Update(uploadCmdList, positionData, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, 0, 1);

	D3D12_SUBRESOURCE_DATA colorData{};
	colorData.pData = data.mColors.data();
	colorData.RowPitch = sizeof(glm::vec4);
	colorData.SlicePitch = sizeof(glm::vec4) * vertexCount;

	data.mVertexColorBuffer->Update(uploadCmdList, colorData, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, 0, 1);

	engineDevice.SubmitUploadCommands();

	// Draw the lines
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST); 
	commandList->SetPipelineState(mDebugPipeline.Get());

	gpuWorld.GetCameraBuffer().Bind(commandList, 0, 0, frameIndex);

	commandList->IASetVertexBuffers(0, 1, &data.mVertexPositionBufferView);
	commandList->IASetVertexBuffers(1, 1, &data.mVertexColorBufferView);
	commandList->DrawInstanced(vertexCount, 1, 0, 0);

	memset(data.mPositions.data(), 0, sizeof(glm::vec3) * vertexCount);
	memset(data.mColors.data(), 0, sizeof(glm::vec4) * vertexCount);
	data.mLineCount = 0;
}