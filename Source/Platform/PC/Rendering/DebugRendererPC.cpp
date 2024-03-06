#include "Precomp.h"
#ifdef PLATFORM_WINDOWS
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

class Engine::DebugRenderer::Impl
{
public:
    Impl();
    bool AddLine(const glm::vec3& from, const glm::vec3& to, const glm::vec4& color);
    void Render(const World& world);

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
	Device& engineDevice = Device::Get();
	FileIO& fileIO = FileIO::Get();
	ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());
	ID3D12GraphicsCommandList4* uploadCmdList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetUploadCommandList());

	mImpl = std::make_unique<Impl>();
	std::string shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/DebugVertex.hlsl");
	ComPtr<ID3DBlob> v = DXPipeline::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
	shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/DebugPixel.hlsl");
	ComPtr<ID3DBlob> p = DXPipeline::ShaderToBlob(shaderPath.c_str(), "ps_5_0", "main");
	mImpl->mDebugPipeline = std::make_unique<DXPipeline>();
	mImpl->mDebugPipeline->AddInput("POSITION", DXGI_FORMAT_R32G32B32A32_FLOAT, 0);
	mImpl->mDebugPipeline->SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), p->GetBufferPointer(), p->GetBufferSize());
	mImpl->mDebugPipeline->CreatePipeline(device, reinterpret_cast<DXSignature*>(engineDevice.GetSignature()), L"SKYBOX SIGNATURE");

	std::vector<glm::vec3> positions(2);
	positions[0] = glm::vec3(-1.f, 0.f, 0.f);
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

	mImpl->mLineColorBuffer = std::make_unique<DXConstBuffer>(device, sizeof(glm::vec3), MAX_LINES, "Lines color const buffer", FRAME_BUFFER_COUNT);
	mImpl->mLineMatrixBuffer = std::make_unique<DXConstBuffer>(device, sizeof(glm::mat4x4), MAX_LINES, "Lines matrix const buffer", FRAME_BUFFER_COUNT);
	mImpl->mCameraMatrixBuffer = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::DXMatrixInfo), 1, "Matrix buffer default shader", FRAME_BUFFER_COUNT);

	engineDevice.SubmitUploadCommands();
}

Engine::DebugRenderer::~DebugRenderer()
{
}

void Engine::DebugRenderer::AddLine(const glm::vec3& from, const glm::vec3& to, const glm::vec4& color)
{

    mImpl->AddLine(from, to, color);
}

void Engine::DebugRenderer::AddSphere(const glm::vec3&, float, const glm::vec4&)
{
}

void Engine::DebugRenderer::AddCube(const glm::vec3&, float)
{
}

void Engine::DebugRenderer::AddPlane(const glm::vec3&, const glm::vec3&)
{
}

void Engine::DebugRenderer::Render(const World& world)
{
	mImpl->Render(world);
}

Engine::DebugRenderer::Impl::Impl()
{
}

bool Engine::DebugRenderer::Impl::AddLine(const glm::vec3& from, const glm::vec3& to, const glm::vec4& color)
{
	// Initial line direction (normalized)
	glm::vec3 initialDir = glm::vec3(1, 0, 0); // From (-1, 0, 0) to (1, 0, 0)

	// Target line direction
	glm::vec3 targetDir = glm::normalize(to - from);

	// Scale factor (the length of the target line)
	float scale = glm::length(to - from) / 2.0f; // Initial line length is 2

	// Calculate the rotation axis and angle
	glm::vec3 rotationAxis = glm::cross(initialDir, targetDir);
	float dotProduct = glm::dot(initialDir, targetDir);
	float angle = acos(dotProduct); // Angle in radians

	// Create rotation matrix
	glm::mat4 rotation = glm::mat4(1.0f); // Identity matrix
	if(glm::length(rotationAxis) != 0) // To avoid division by zero
		rotation = glm::rotate(rotation, angle, glm::normalize(rotationAxis));

	// Create scale matrix
	glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(scale, scale, scale));

	// Create translation matrix
	glm::mat4 translation = glm::translate(glm::mat4(1.0f), from);

	// Compose the model matrix: first scale, then rotate, and finally translate
	glm::mat4 modelMatrix = translation * rotation * scaleMatrix;

	mModelMats.push_back(glm::transpose(modelMatrix));
	mColors.push_back(color);

    return true;
}

void Engine::DebugRenderer::Impl::Render(const World& world)
{
	Device& engineDevice = Device::Get();
	ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
	std::shared_ptr<DXDescHeap> resourceHeap = engineDevice.GetDescriptorHeap(RESOURCE_HEAP);
	int frameIndex = engineDevice.GetFrameIndex();

	//GET WORLD
	const auto optionalEntityCameraPair = world.GetRenderer().GetMainCamera();
	ASSERT_LOG(optionalEntityCameraPair.has_value(), "DX12 draw requests have been made, but they cannot be cleared as there is no camera to draw them to");

	//UPDATE CAMERA
	const auto camera = optionalEntityCameraPair->second;
	InfoStruct::DXMatrixInfo matrixInfo;
	matrixInfo.pm = glm::transpose(glm::scale(camera.GetProjection(), glm::vec3(1.f, -1.f, 1.f)));
	matrixInfo.vm = glm::transpose(camera.GetView());
	matrixInfo.ipm = glm::inverse(matrixInfo.pm);
	matrixInfo.ivm = glm::inverse(matrixInfo.vm);
	mCameraMatrixBuffer->Update(&matrixInfo, sizeof(InfoStruct::DXMatrixInfo), 0, frameIndex);

	//BIND CONSTANT BUFFERS
	mCameraMatrixBuffer->Bind(commandList, 0, 0, frameIndex);

	for (size_t i = 0; i < mModelMats.size(); i++) {
		mLineMatrixBuffer->Update(mModelMats.data(), sizeof(glm::mat4x4), static_cast<int>(i), frameIndex);
		mLineMatrixBuffer->Bind(commandList, 2, static_cast<int>(i), frameIndex);

		commandList->IASetVertexBuffers(0, 1, &mVertexBufferView);
		commandList->DrawInstanced(2, 1, 0, 0);
	}
}
#endif