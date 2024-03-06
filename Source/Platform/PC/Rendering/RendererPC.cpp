#include "Precomp.h"
#include "Platform/PC/Rendering/RendererPC.h"
#include "Core/Device.h"
#include "Core/FileIO.h"

#include "Platform/PC/Rendering/DX12Classes/DXSignature.h"
#include "Platform/PC/Rendering/DX12Classes/DXConstBuffer.h"
#include "Platform/PC/Rendering/DX12Classes/DXPipeline.h"
#include "Platform/PC/Rendering/DX12Classes/DXDescHeap.h"
#include "Platform/PC/Rendering/DX12Classes/DXResource.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TransformComponent.h"
#include "World/Registry.h"
#include "World/World.h"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"

#include "World/WorldRenderer.h"
#include "Components/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Assets/Material.h"
#include "Assets/Texture.h"
#include "Assets/StaticMesh.h"
#include "Utilities/DebugRenderer.h"

Engine::Renderer::Renderer()
{
    Device& engineDevice = Device::Get();
    ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());

    //CREATE PBR PIPELINE
    FileIO& fileIO = FileIO::Get();
    std::string shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/PBRVertex.hlsl");
    ComPtr<ID3DBlob> v = DXPipeline::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/PBRPixel.hlsl");
    ComPtr<ID3DBlob> p = DXPipeline::ShaderToBlob(shaderPath.c_str(), "ps_5_0", "main");
    mPBRPipeline = std::make_unique<DXPipeline>();
    CD3DX12_RASTERIZER_DESC rast = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    rast.CullMode = D3D12_CULL_MODE_FRONT;
    mPBRPipeline->AddInput("POSITION", DXGI_FORMAT_R32G32B32A32_FLOAT, 0);
    mPBRPipeline->AddInput("NORMAL", DXGI_FORMAT_R32G32B32A32_FLOAT, 1);
    mPBRPipeline->AddInput("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT, 2);
    mPBRPipeline->AddInput("TANGENT", DXGI_FORMAT_R32G32B32_FLOAT, 3);
    mPBRPipeline->AddRenderTarget(DXGI_FORMAT_R8G8B8A8_UNORM);
    mPBRPipeline->SetRasterizer(rast);
    mPBRPipeline->SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), p->GetBufferPointer(), p->GetBufferSize());
    mPBRPipeline->CreatePipeline(device, reinterpret_cast<DXSignature*>(engineDevice.GetSignature()), L"PBR RENDER PIPELINE");

    //CREATE CONSTANT BUFFERS
    mConstBuffers[CAM_MATRIX_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::DXMatrixInfo), 1, "Matrix buffer default shader", FRAME_BUFFER_COUNT);
    mConstBuffers[LIGHT_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::DXLightInfo), 1, "Point light buffer", FRAME_BUFFER_COUNT);
    mConstBuffers[MATERIAL_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::DXMaterialInfo), MAX_MESHES + 2, "Material info data", FRAME_BUFFER_COUNT);
    mConstBuffers[MODEL_MATRIX_CB] = std::make_unique<DXConstBuffer>(device, sizeof(glm::mat4x4), MAX_MESHES, "Mesh matrix data", FRAME_BUFFER_COUNT);

    mDebugRenderer = std::make_unique<DebugRenderer>();
}

void Engine::Renderer::Render(const World& world)
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
    //matrixInfo.pm = camera.GetProjection();
    matrixInfo.pm = glm::transpose(glm::scale(camera.GetProjection(), glm::vec3(1.f, -1.f, 1.f)));
    matrixInfo.vm = glm::transpose(camera.GetView());

    matrixInfo.ipm = glm::inverse(matrixInfo.pm);
    matrixInfo.ivm = glm::inverse(matrixInfo.vm);
    mConstBuffers[CAM_MATRIX_CB]->Update(&matrixInfo, sizeof(InfoStruct::DXMatrixInfo), 0, frameIndex);

    //UPDATE LIGHTS
    const auto pointLightView = world.GetRegistry().View<const PointLightComponent, const TransformComponent>();
    int pointLightCounter = 0;
    for (auto [entity, lightComponent, transform] : pointLightView.each()) {
        if (pointLightCounter < MAX_LIGHTS) {
            lights.mPointLights[pointLightCounter].mPosition = glm::vec4(transform.GetWorldPosition(),1.f);
            lights.mPointLights[pointLightCounter].mColorAndIntensity = glm::vec4(lightComponent.mColor, lightComponent.mIntensity);
            lights.mPointLights[pointLightCounter].mRadius = lightComponent.mRange;
        }
        else {
            LOG(LogCore, Warning, ("Maximum (%i) of point lights has been surpassed.", MAX_LIGHTS));
            break;
        }
        pointLightCounter++;
    }
    const auto dirLightView = world.GetRegistry().View<const DirectionalLightComponent, const TransformComponent>();
    int dirLightCounter = 0;
    for (auto [entity, lightComponent, transform] : dirLightView.each()) {
        if (dirLightCounter < MAX_LIGHTS) {
            glm::quat quatRotation = transform.GetLocalOrientation();
            glm::vec3 baseDir = glm::vec3(0, 0, 1);
            glm::vec3 lightDirection = quatRotation * baseDir;

            lights.mDirLights[dirLightCounter].mDir = glm::vec4(lightDirection, 1.f);
            lights.mDirLights[dirLightCounter].mColorAndIntensity = glm::vec4(lightComponent.mColor, lightComponent.mIntensity);
        }
        else {
            LOG(LogCore, Warning, ("Maximum (%i) of directional lights has been surpassed.", MAX_LIGHTS));
            break;
        }
        dirLightCounter++;
    }

    mConstBuffers[LIGHT_CB]->Update(&lights, sizeof(InfoStruct::DXLightInfo), 0, frameIndex);
    commandList->SetPipelineState(mPBRPipeline->GetPipeline().Get());

    //BIND CONSTANT BUFFERS
    mConstBuffers[LIGHT_CB]->Bind(commandList, 1, 0, frameIndex);
    mConstBuffers[CAM_MATRIX_CB]->Bind(commandList, 0, 0, frameIndex);
    mConstBuffers[CAM_MATRIX_CB]->Bind(commandList, 4, 0, frameIndex);

    ID3D12DescriptorHeap* descriptorHeaps[] = {resourceHeap->Get()};
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    const auto view = world.GetRegistry().View<const StaticMeshComponent, const TransformComponent>();
    int meshCounter = 0;

    for (auto [entity, staticMeshComponent, transform] : view.each())
    {
        //UPDATE AND BIND MODEL MATRIX
        glm::mat4x4 modelMatrix = glm::transpose(transform.GetWorldMatrix());
        mConstBuffers[MODEL_MATRIX_CB]->Update(&modelMatrix, sizeof(glm::mat4x4), meshCounter, frameIndex);
        mConstBuffers[MODEL_MATRIX_CB]->Bind(commandList, 2, meshCounter, frameIndex);

        //UPDATE AND BIND MATERIAL INFO
        InfoStruct::DXMaterialInfo materialInfo;
        if (staticMeshComponent.mMaterial != nullptr) {
            materialInfo.colorFactor = { staticMeshComponent.mMaterial->mBaseColorFactor.r,
                staticMeshComponent.mMaterial->mBaseColorFactor.g,
                staticMeshComponent.mMaterial->mBaseColorFactor.b,
                0.f };
            materialInfo.emissiveFactor = { staticMeshComponent.mMaterial->mEmissiveFactor.r,
                staticMeshComponent.mMaterial->mEmissiveFactor.g,
                staticMeshComponent.mMaterial->mEmissiveFactor.b,
                0.f };
            materialInfo.metallicFactor = staticMeshComponent.mMaterial->mMetallicFactor;
            materialInfo.roughnessFactor = staticMeshComponent.mMaterial->mRoughnessFactor;
            materialInfo.normalScale = staticMeshComponent.mMaterial->mNormalScale;
            materialInfo.useColorTex = staticMeshComponent.mMaterial->mBaseColorTexture != nullptr;
            materialInfo.useEmissiveTex = staticMeshComponent.mMaterial->mEmissiveTexture != nullptr;
            materialInfo.useMetallicRoughnessTex = staticMeshComponent.mMaterial->mMetallicRoughnessTexture != nullptr;
            materialInfo.useNormalTex = staticMeshComponent.mMaterial->mNormalTexture != nullptr;
            materialInfo.useOcclusionTex = staticMeshComponent.mMaterial->mOcclusionTexture != nullptr;
            
            //BIND TEXTURES
            if (materialInfo.useColorTex)
            {
                resourceHeap->BindToGraphics(commandList, 5, staticMeshComponent.mMaterial->mBaseColorTexture->GetIndex());
            }
            if (materialInfo.useEmissiveTex)
            {
                resourceHeap->BindToGraphics(commandList, 6, staticMeshComponent.mMaterial->mEmissiveTexture->GetIndex());
            }
            if (materialInfo.useMetallicRoughnessTex)
            {
                resourceHeap->BindToGraphics(commandList, 7, staticMeshComponent.mMaterial->mMetallicRoughnessTexture->GetIndex());
            }
            if (materialInfo.useNormalTex)
            {
                resourceHeap->BindToGraphics(commandList, 8, staticMeshComponent.mMaterial->mNormalTexture->GetIndex());
            }
            if (materialInfo.useOcclusionTex)
            {
                resourceHeap->BindToGraphics(commandList, 9, staticMeshComponent.mMaterial->mOcclusionTexture->GetIndex());
            }
        }
        else {
            materialInfo.colorFactor = {0.f, 0.f, 0.f, 0.f };
            materialInfo.emissiveFactor = {0.f, 0.f, 0.f, 0.f };
            materialInfo.metallicFactor = 0.f;
            materialInfo.roughnessFactor = 0.f;
            materialInfo.normalScale = 0.f;
            materialInfo.useColorTex = false;
            materialInfo.useEmissiveTex = false;
            materialInfo.useMetallicRoughnessTex = false;
            materialInfo.useNormalTex = false;
            materialInfo.useOcclusionTex = false;

        }
        mConstBuffers[MATERIAL_CB]->Update(&materialInfo, sizeof(InfoStruct::DXMaterialInfo), meshCounter, frameIndex);
        mConstBuffers[MATERIAL_CB]->Bind(commandList, 3, meshCounter, frameIndex);

        //DRAW THE MESH
        staticMeshComponent.mStaticMesh->DrawMesh();

        meshCounter++;
    }
}

Engine::MetaType Engine::Renderer::Reflect()
{
    return MetaType{ MetaType::T<Renderer>{}, "Renderer", MetaType::Base<System>{} };
}
