#include "Precomp.h"
#include "Utilities/DebugRenderer.h"
#include "Core/FileIO.h"
#include "Core/Device.h"
#include "Platform/PC/Rendering/DX12Classes/DXDefines.h"
#include "Platform/PC/Rendering/DX12Classes/DXResource.h"
#include "Platform/PC/Rendering/DX12Classes/DXPipeline.h"
#include "Platform/PC/Rendering/DX12Classes/DXConstBuffer.h"
#include "Platform/PC/Rendering/RendererPC.h"
#include "World/World.h"
#include "World/WorldRenderer.h"
#include "World/Registry.h"
#include "Components/CameraComponent.h"
#include <memory>

#include "World/World.h"
#include "World/Registry.h"
#include "Components/TransformComponent.h"
#include "Components/CameraComponent.h"

class Engine::DebugRenderer::Impl
{
public:
	Impl() = default;
    bool AddLine(const glm::vec3& from, const glm::vec3& to, const glm::vec4& color);
    void Render(const glm::mat4& view, const glm::mat4& projection);

	D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
    std::vector<glm::mat4x4> mModelMats;
    std::vector<glm::vec4> mColors;

	std::unique_ptr<DXPipeline> mDebugPipeline;
	std::unique_ptr<DXResource> mVertexBuffer;
	std::unique_ptr<DXConstBuffer> mLineColorBuffer;
	std::unique_ptr<DXConstBuffer> mLineMatrixBuffer;
	std::unique_ptr<DXConstBuffer> mCameraMatrixBuffer;
};

Engine::DebugRenderer::DebugRenderer()
{
	if (Device::IsHeadless())
	{
		return;
	}

	Device& engineDevice = Device::Get();
	mImpl = std::make_unique<Impl>();

	FileIO& fileIO = FileIO::Get();
	ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());
	ID3D12GraphicsCommandList4* uploadCmdList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetUploadCommandList());

	std::string shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/DebugVertex.hlsl");
	ComPtr<ID3DBlob> v = DXPipeline::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
	shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/DebugPixel.hlsl");
	ComPtr<ID3DBlob> p = DXPipeline::ShaderToBlob(shaderPath.c_str(), "ps_5_0", "main");
	mImpl->mDebugPipeline = std::make_unique<DXPipeline>();
	mImpl->mDebugPipeline->AddInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0);
	mImpl->mDebugPipeline->SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), p->GetBufferPointer(), p->GetBufferSize());
	mImpl->mDebugPipeline->SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
	mImpl->mDebugPipeline->CreatePipeline(device, reinterpret_cast<DXSignature*>(engineDevice.GetSignature()), L"SKYBOX SIGNATURE");

	std::vector<glm::vec3> positions(2);
	positions[0] = glm::vec3(0.f, 0.f, 0.f);
	positions[1] = glm::vec3(1.f, 0.f, 0.f);

	engineDevice.StartUploadCommands();
	int vertexCount = 2;
	int vBufferSize = sizeof(float) * vertexCount * 3;

	mImpl->mVertexBuffer = std::make_unique<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), CD3DX12_RESOURCE_DESC::Buffer(vBufferSize), nullptr, "Line Vertex resource buffer");
	
	D3D12_SUBRESOURCE_DATA vData = {};
	vData.pData = positions.data();
	vData.RowPitch = sizeof(float) * 3;
	vData.SlicePitch = vBufferSize;

	mImpl->mVertexBuffer->CreateUploadBuffer(device, vBufferSize, 0);
	mImpl->mVertexBuffer->Update(uploadCmdList, vData, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, 0, 1);

	mImpl->mVertexBufferView.BufferLocation = mImpl->mVertexBuffer->GetResource()->GetGPUVirtualAddress();
	mImpl->mVertexBufferView.StrideInBytes = sizeof(float) * 3;
	mImpl->mVertexBufferView.SizeInBytes = vBufferSize;

	mImpl->mLineColorBuffer = std::make_unique<DXConstBuffer>(device, sizeof(glm::vec4), MAX_LINES, "Lines color const buffer", FRAME_BUFFER_COUNT);
	mImpl->mLineMatrixBuffer = std::make_unique<DXConstBuffer>(device, sizeof(glm::mat4x4), MAX_LINES, "Lines matrix const buffer", FRAME_BUFFER_COUNT);
	mImpl->mCameraMatrixBuffer = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::DXMatrixInfo), 1, "Matrix buffer default shader", FRAME_BUFFER_COUNT);

	engineDevice.SubmitUploadCommands();
}

Engine::DebugRenderer::~DebugRenderer() = default;

void Engine::DebugRenderer::AddLine(DebugCategory::Enum category, const glm::vec3& from, const glm::vec3& to, const glm::vec4& color) const
{
	if (!Device::IsHeadless()
		&& (sDebugCategoryFlags & category) != 0)
	{
		mImpl->AddLine(from, to, color);
	}
}

void Engine::DebugRenderer::Render(const World& world)
{
    const auto cameraView = world.GetRegistry().View<const TransformComponent, const CameraComponent>();

	const auto optionalEntityCameraPair = world.GetRenderer().GetMainCamera();
    ASSERT_LOG(optionalEntityCameraPair.has_value(), "DX12 draw requests have been made, but they cannot be cleared as there is no camera to draw them to");

	const auto camera = optionalEntityCameraPair->second;
    mImpl->Render(camera.GetView(), camera.GetProjection());
}

bool Engine::DebugRenderer::Impl::AddLine(const glm::vec3& from, const glm::vec3& to, const glm::vec4& color)
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

	mModelMats.push_back(glm::transpose(modelMatrix));
	mColors.push_back(color);

    return true;
}

void Engine::DebugRenderer::Impl::Render(const glm::mat4& view, const glm::mat4& projection)
{
	Device& engineDevice = Device::Get();
	ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
	std::shared_ptr<DXDescHeap> resourceHeap = engineDevice.GetDescriptorHeap(RESOURCE_HEAP);
	int frameIndex = engineDevice.GetFrameIndex();

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST); 
	commandList->SetPipelineState(mDebugPipeline->GetPipeline().Get());

	//UPDATE CAMERA
	InfoStruct::DXMatrixInfo matrixInfo;
	matrixInfo.pm = glm::transpose(glm::scale(projection, glm::vec3(1.f, -1.f, 1.f)));
	matrixInfo.vm = glm::transpose(view);
	matrixInfo.ipm = glm::inverse(matrixInfo.pm);
	matrixInfo.ivm = glm::inverse(matrixInfo.vm);
	mCameraMatrixBuffer->Update(&matrixInfo, sizeof(InfoStruct::DXMatrixInfo), 0, frameIndex);

	//BIND CONSTANT BUFFERS
	mCameraMatrixBuffer->Bind(commandList, 0, 0, frameIndex);

	for (size_t i = 0; i < mModelMats.size(); i++) {
		mLineMatrixBuffer->Update(&mModelMats[i], sizeof(glm::mat4x4), static_cast<int>(i), frameIndex);
		mLineMatrixBuffer->Bind(commandList, 2, static_cast<int>(i), frameIndex);

		mLineColorBuffer->Update(&mColors[i], sizeof(glm::vec4), static_cast<int>(i), frameIndex);
		mLineColorBuffer->Bind(commandList, 1, static_cast<int>(i), frameIndex);

		commandList->IASetVertexBuffers(0, 1, &mVertexBufferView);
		commandList->DrawInstanced(2, 1, 0, 0);
	}
	mModelMats.clear();
	mColors.clear();
}