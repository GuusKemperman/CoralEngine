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

class Engine::DebugRenderer::Impl
{
public:
	Impl() = default;
	~Impl() = default;
    bool AddLine(const World& world, const glm::vec3& from, const glm::vec3& to, const glm::vec4& color);
    void Render(GPUWorld& gpuWorld);

	std::unique_ptr<DXPipeline> mDebugPipeline;
};

Engine::DebugRenderer::DebugRenderer()
{
	Device& engineDevice = Device::Get();
	mImpl = std::make_unique<Impl>();

	FileIO& fileIO = FileIO::Get();
	ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());

	std::string shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/DebugVertex.hlsl");
	ComPtr<ID3DBlob> v = DXPipeline::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
	shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/DebugPixel.hlsl");
	ComPtr<ID3DBlob> p = DXPipeline::ShaderToBlob(shaderPath.c_str(), "ps_5_0", "main");
	mImpl->mDebugPipeline = std::make_unique<DXPipeline>();
	mImpl->mDebugPipeline->AddInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0);
	mImpl->mDebugPipeline->SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), p->GetBufferPointer(), p->GetBufferSize());
	mImpl->mDebugPipeline->SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
	mImpl->mDebugPipeline->CreatePipeline(device, reinterpret_cast<DXSignature*>(engineDevice.GetSignature()), L"Debug line pipeline");
}

Engine::DebugRenderer::~DebugRenderer() = default;

void Engine::DebugRenderer::AddLine(const World& world, DebugCategory::Enum category, const glm::vec3& from, const glm::vec3& to, const glm::vec4& color) const
{
	if ((sDebugCategoryFlags & category) != 0)
	{
		mImpl->AddLine(world, from, to, color);
	}
}

void Engine::DebugRenderer::Render(const World& world)
{
    mImpl->Render(world.GetGPUWorld());
}

bool Engine::DebugRenderer::Impl::AddLine(const World& world, const glm::vec3& from, const glm::vec3& to, const glm::vec4& color)
{
	// Calculate the scale as the distance between 'from' and 'to'
	float scaleLength = glm::length(to - from);

	// Calculate the direction vector of the target line and normalize it
	glm::vec3 direction = glm::normalize(to - from);

	// Initial direction vector of the line is along the x-axis (1, 0, 0)
	glm::vec3 initialDirection = glm::vec3(1.f, 0.f, 0.f);

	// Calculate rotation quaternion from initial direction to target direction
	glm::quat rotationQuat = glm::rotation(initialDirection, direction);

	// Convert quaternion to a rotation matrix
	glm::mat4 rotationMat = glm::toMat4(rotationQuat);

	// Scale, Rotate, then Translate
	glm::mat4 scaleMat = glm::scale(glm::mat4x4(1.f), glm::vec3(scaleLength)); // Uniform scale
	glm::mat4 translateMat = glm::translate(glm::mat4x4(1.f), from);

	// Combine transformations: T * R * S
	glm::mat4 modelMatrix = translateMat * rotationMat * scaleMat;

	GPUWorld& gpuWorld = world.GetGPUWorld();
	GPUWorld::DebugRenderingData& data = gpuWorld.GetDebugRenderingData();

	data.mModelMats.push_back(glm::transpose(modelMatrix));
	data.mColors.push_back(color);

    return true;
}

void Engine::DebugRenderer::Impl::Render(GPUWorld& gpuWorld)
{
	Device& engineDevice = Device::Get();
	ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
	std::shared_ptr<DXDescHeap> resourceHeap = engineDevice.GetDescriptorHeap(RESOURCE_HEAP);
	int frameIndex = engineDevice.GetFrameIndex();

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST); 
	commandList->SetPipelineState(mDebugPipeline->GetPipeline().Get());

	gpuWorld.GetCameraBuffer().Bind(commandList, 0, 0, frameIndex);

	GPUWorld::DebugRenderingData& data = gpuWorld.GetDebugRenderingData();

	for (size_t i = 0; i < data.mModelMats.size(); i++)
	{
		data.mLineMatrixBuffer->Update(&data.mModelMats[i], sizeof(glm::mat4x4), static_cast<int>(i), frameIndex);
		data.mLineMatrixBuffer->Bind(commandList, 2, static_cast<int>(i), frameIndex);

		data.mLineColorBuffer->Update(&data.mColors[i], sizeof(glm::vec4), static_cast<int>(i), frameIndex);
		data.mLineColorBuffer->Bind(commandList, 1, static_cast<int>(i), frameIndex);

		commandList->IASetVertexBuffers(0, 1, &data.mVertexBufferView);
		commandList->DrawInstanced(2, 1, 0, 0);
	}

	data.mModelMats.clear();
	data.mColors.clear();
}