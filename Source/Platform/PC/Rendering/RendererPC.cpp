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
#include "Components/SkinnedMeshComponent.h"
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
#include "Assets/SkinnedMesh.h"
#include "Utilities/DebugRenderer.h"

Engine::Renderer::Renderer()
{
    if (Device::IsHeadless())
    {
        return;
    }

    Device& engineDevice = Device::Get();

    ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());

    //CREATE PBR PIPELINE
    FileIO& fileIO = FileIO::Get();
    std::string shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/PBRVertex.hlsl");
    ComPtr<ID3DBlob> v = DXPipeline::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/PBRPixel.hlsl");
    ComPtr<ID3DBlob> p = DXPipeline::ShaderToBlob(shaderPath.c_str(), "ps_5_0", "main");

    mPBRPipeline = std::make_unique<DXPipeline>();
    CD3DX12_DEPTH_STENCIL_DESC depth = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    depth.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;

    mPBRPipeline->AddInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0);
    mPBRPipeline->AddInput("NORMAL", DXGI_FORMAT_R32G32B32_FLOAT, 1);
    mPBRPipeline->AddInput("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT, 2);
    mPBRPipeline->AddInput("TANGENT", DXGI_FORMAT_R32G32B32_FLOAT, 3);
    mPBRPipeline->AddRenderTarget(DXGI_FORMAT_R8G8B8A8_UNORM);
    mPBRPipeline->SetDepthState(depth);
    mPBRPipeline->SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), p->GetBufferPointer(), p->GetBufferSize());
    mPBRPipeline->CreatePipeline(device, reinterpret_cast<DXSignature*>(engineDevice.GetSignature()), L"PBR RENDER PIPELINE");

    //CREATE PBR SKINNED PIPELINE
    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/PBRVertexSkinned.hlsl");
    v = DXPipeline::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/PBRPixel.hlsl");
    p = DXPipeline::ShaderToBlob(shaderPath.c_str(), "ps_5_0", "main");
    mPBRSkinnedPipeline = std::make_unique<DXPipeline>();
    mPBRSkinnedPipeline->AddInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0);
    mPBRSkinnedPipeline->AddInput("NORMAL", DXGI_FORMAT_R32G32B32_FLOAT, 1);
    mPBRSkinnedPipeline->AddInput("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT, 2);
    mPBRSkinnedPipeline->AddInput("TANGENT", DXGI_FORMAT_R32G32B32_FLOAT, 3);
    mPBRSkinnedPipeline->AddInput("BONEIDS", DXGI_FORMAT_R32G32B32A32_SINT, 4);
    mPBRSkinnedPipeline->AddInput("BONEWEIGHTS", DXGI_FORMAT_R32G32B32A32_FLOAT, 5);
    mPBRSkinnedPipeline->AddRenderTarget(DXGI_FORMAT_R8G8B8A8_UNORM);
    mPBRSkinnedPipeline->SetDepthState(depth);
    mPBRSkinnedPipeline->SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), p->GetBufferPointer(), p->GetBufferSize());
    mPBRSkinnedPipeline->CreatePipeline(device, reinterpret_cast<DXSignature*>(engineDevice.GetSignature()), L"PBR SKINNED RENDER PIPELINE");
    
    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/ZVertex.hlsl");
    v = DXPipeline::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
    mZPipeline = std::make_unique<DXPipeline>();
    mZPipeline->AddInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0);
    mZPipeline->AddRenderTarget(DXGI_FORMAT_R8G8B8A8_UNORM);
    mZPipeline->SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), nullptr, 0);
    mZPipeline->CreatePipeline(device, reinterpret_cast<DXSignature*>(engineDevice.GetSignature()), L"DEPTH RENDER PIPELINE");

    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/ZSkinnedVertex.hlsl");
    v = DXPipeline::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
    mZSkinnedPipeline = std::make_unique<DXPipeline>();
    mZSkinnedPipeline->AddInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0);
    mZSkinnedPipeline->AddInput("BONEIDS", DXGI_FORMAT_R32G32B32A32_SINT, 1);
    mZSkinnedPipeline->AddInput("BONEWEIGHTS", DXGI_FORMAT_R32G32B32A32_FLOAT, 2);
    mZSkinnedPipeline->AddRenderTarget(DXGI_FORMAT_R8G8B8A8_UNORM);
    mZSkinnedPipeline->SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), nullptr, 0);
    mZSkinnedPipeline->CreatePipeline(device, reinterpret_cast<DXSignature*>(engineDevice.GetSignature()), L"SKINNED DEPTH RENDER PIPELINE");

    //CREATE CONSTANT BUFFERS
    mConstBuffers[CAM_MATRIX_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::DXMatrixInfo), 1, "Matrix buffer default shader", FRAME_BUFFER_COUNT);
    mConstBuffers[LIGHT_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::DXLightInfo), 1, "Point light buffer", FRAME_BUFFER_COUNT);
    mConstBuffers[MODEL_INDEX_CB] = std::make_unique<DXConstBuffer>(device, sizeof(int), MAX_MESHES + 2, "Model index", FRAME_BUFFER_COUNT);
    mConstBuffers[MODEL_MATRIX_CB] = std::make_unique<DXConstBuffer>(device, sizeof(glm::mat4x4) * 2, MAX_MESHES, "Mesh matrix data", FRAME_BUFFER_COUNT);
    mConstBuffers[FINAL_BONE_MATRIX_CB] = std::make_unique<DXConstBuffer>(device, sizeof(glm::mat4x4) * MAX_BONES, MAX_SKINNED_MESHES, "Skinned Mesh Bone Matrices", FRAME_BUFFER_COUNT);
    //CREATE STRUCTURED BUFFERS
    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(InfoStruct::DXMaterialInfo) * (MAX_MESHES + 2), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mStructuredBuffers[DXStructuredBuffers::MATERIAL_SB] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "MATERIAL STRUCTURED BUFFER");
    mStructuredBuffers[DXStructuredBuffers::MATERIAL_SB]->CreateUploadBuffer(device, sizeof(InfoStruct::DXMaterialInfo)* (MAX_MESHES + 2), 0);

    D3D12_SHADER_RESOURCE_VIEW_DESC  srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.StructureByteStride = sizeof(InfoStruct::DXMaterialInfo);
    srvDesc.Buffer.NumElements = MAX_MESHES +2;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    materialHeapSlot = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateResource(mStructuredBuffers[DXStructuredBuffers::MATERIAL_SB].get(), &srvDesc);
    materials = std::vector<InfoStruct::DXMaterialInfo>(MAX_MESHES + 2);
}

static void SendMaterialToGPUIfReady(const Engine::Material& mat);

void Engine::Renderer::Render(const World& world)
{
    // I'm not sure why, because I (Guus), know nothing of dx12, but dx12 does not like it
    // if we are sending textures to the GPU in the RENDERING pass. Soo my solution was to
    // do it here instead. The only downside is that we have to iterate over the static meshes
    // twice, but in entt this is surprisingly fast.
    { 
        const auto view = world.GetRegistry().View<const StaticMeshComponent>();

        for (auto [entity, staticMeshComponent] : view.each())
        {
            // It won't be rendered anyway,
            // so let's not bother finalising
            // the loading process.
            if (staticMeshComponent.mMaterial != nullptr
                && staticMeshComponent.mStaticMesh != nullptr)
            {
                SendMaterialToGPUIfReady(*staticMeshComponent.mMaterial);            
            }
        }
    }

    {
        const auto view = world.GetRegistry().View<const SkinnedMeshComponent>();

        for (auto [entity, skinnedMeshComponent] : view.each())
        {
            // It won't be rendered anyway,
            // so let's not bother finalising
            // the loading process.
            if (skinnedMeshComponent.mMaterial != nullptr
                && skinnedMeshComponent.mSkinnedMesh != nullptr)
            {
            SendMaterialToGPUIfReady(*skinnedMeshComponent.mMaterial);
            }

        }
    }

    Device& engineDevice = Device::Get();
    ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
    std::shared_ptr<DXDescHeap> resourceHeap = engineDevice.GetDescriptorHeap(RESOURCE_HEAP);
    int frameIndex = engineDevice.GetFrameIndex();

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); 

    //GET WORLD
    const auto optionalEntityCameraPair = world.GetRenderer().GetMainCamera();
    ASSERT_LOG(optionalEntityCameraPair.has_value(), "DX12 draw requests have been made, but they cannot be cleared as there is no camera to draw them to");

    //UPDATE CAMERA
    const auto camera = optionalEntityCameraPair->second;
    InfoStruct::DXMatrixInfo matrixInfo;
    matrixInfo.pm = glm::transpose(camera.GetProjection());
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

    //BIND CONSTANT BUFFERS
    mConstBuffers[LIGHT_CB]->Bind(commandList, 1, 0, frameIndex);
    mConstBuffers[CAM_MATRIX_CB]->Bind(commandList, 0, 0, frameIndex);
    mConstBuffers[CAM_MATRIX_CB]->Bind(commandList, 4, 0, frameIndex);

    ID3D12DescriptorHeap* descriptorHeaps[] = {resourceHeap->Get()};
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    int meshCounter = 0;
    commandList->SetPipelineState(mZPipeline->GetPipeline().Get());

    {
        const auto view = world.GetRegistry().View<const StaticMeshComponent, const TransformComponent>();
        //DEPTH PRE PASS
        for (auto [entity, staticMeshComponent, transform] : view.each()) {
        glm::mat4x4 modelMatrices[2]{};
        modelMatrices[0] = glm::transpose(transform.GetWorldMatrix());
        modelMatrices[1] = glm::transpose(glm::inverse(modelMatrices[0]));
        mConstBuffers[MODEL_MATRIX_CB]->Update(&modelMatrices, sizeof(glm::mat4x4) * 2, meshCounter, frameIndex);
        mConstBuffers[MODEL_MATRIX_CB]->Bind(commandList, 2, meshCounter, frameIndex);

        if (!staticMeshComponent.mStaticMesh)
            continue;

        staticMeshComponent.mStaticMesh->DrawMeshVertexOnly();
        meshCounter++;
        }
    }

    commandList->SetPipelineState(mZSkinnedPipeline->GetPipeline().Get());

    {
        const auto view = world.GetRegistry().View<const SkinnedMeshComponent, const TransformComponent>();

        for (auto [entity, skinnedMeshComponent, transform] : view.each()) 
        {
            glm::mat4x4 modelMatrices[2]{};
            modelMatrices[0] = glm::transpose(transform.GetWorldMatrix());
            modelMatrices[1] = glm::transpose(glm::inverse(modelMatrices[0]));
            mConstBuffers[MODEL_MATRIX_CB]->Update(&modelMatrices, sizeof(glm::mat4x4) * 2, meshCounter, frameIndex);
            mConstBuffers[MODEL_MATRIX_CB]->Bind(commandList, 2, meshCounter, frameIndex);

            const auto& boneMatrices = skinnedMeshComponent.mFinalBoneMatrices;
            mConstBuffers[FINAL_BONE_MATRIX_CB]->Update(&boneMatrices.at(0), boneMatrices.size() * sizeof(glm::mat4x4), meshCounter, frameIndex);
            mConstBuffers[FINAL_BONE_MATRIX_CB]->Bind(commandList, 5, meshCounter, frameIndex);

            if (!skinnedMeshComponent.mSkinnedMesh)
                continue;

            skinnedMeshComponent.mSkinnedMesh->DrawMeshVertexOnly();
            meshCounter++;
        }
    }

    commandList->SetPipelineState(mPBRPipeline->GetPipeline().Get());

    meshCounter = 0;
    {
    const auto view = world.GetRegistry().View<const StaticMeshComponent, const TransformComponent>();
    //RENDERING
    for (auto [entity, staticMeshComponent, transform] : view.each())
    {
        if (!staticMeshComponent.mStaticMesh)
        {
            continue;
        }

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
            materialInfo.useColorTex = staticMeshComponent.mMaterial->mBaseColorTexture != nullptr && staticMeshComponent.mMaterial->mBaseColorTexture->WasSendToGPU();
            materialInfo.useEmissiveTex = staticMeshComponent.mMaterial->mEmissiveTexture != nullptr && staticMeshComponent.mMaterial->mEmissiveTexture->WasSendToGPU();
            materialInfo.useMetallicRoughnessTex = staticMeshComponent.mMaterial->mMetallicRoughnessTexture != nullptr && staticMeshComponent.mMaterial->mMetallicRoughnessTexture->WasSendToGPU();
            materialInfo.useNormalTex = staticMeshComponent.mMaterial->mNormalTexture != nullptr && staticMeshComponent.mMaterial->mNormalTexture->WasSendToGPU();
            materialInfo.useOcclusionTex = staticMeshComponent.mMaterial->mOcclusionTexture != nullptr && staticMeshComponent.mMaterial->mOcclusionTexture->WasSendToGPU();

            //BIND TEXTURES
            if (materialInfo.useColorTex)
            {
                staticMeshComponent.mMaterial->mBaseColorTexture->BindToGraphics(commandList, 6);
            }
            if (materialInfo.useEmissiveTex)
            {
                staticMeshComponent.mMaterial->mEmissiveTexture->BindToGraphics(commandList, 7);
            }
            if (materialInfo.useMetallicRoughnessTex)
            {
                staticMeshComponent.mMaterial->mMetallicRoughnessTexture->BindToGraphics(commandList, 8);
            }
            if (materialInfo.useNormalTex)
            {
                staticMeshComponent.mMaterial->mNormalTexture->BindToGraphics(commandList, 9);
            }
            if (materialInfo.useOcclusionTex)
            {
                staticMeshComponent.mMaterial->mOcclusionTexture->BindToGraphics(commandList, 10);
            }
        }
        else {
            materialInfo.colorFactor = { 0.f, 0.f, 0.f, 0.f };
            materialInfo.emissiveFactor = { 0.f, 0.f, 0.f, 0.f };
            materialInfo.metallicFactor = 0.f;
            materialInfo.roughnessFactor = 0.f;
            materialInfo.normalScale = 0.f;
            materialInfo.useColorTex = false;
            materialInfo.useEmissiveTex = false;
            materialInfo.useMetallicRoughnessTex = false;
            materialInfo.useNormalTex = false;
            materialInfo.useOcclusionTex = false;

        }
        materials[meshCounter] = materialInfo;

        mConstBuffers[MODEL_MATRIX_CB]->Bind(commandList, 2, meshCounter, frameIndex);

        //UPDATE AND BIND MATERIAL INFO
        mConstBuffers[MODEL_INDEX_CB]->Update(&meshCounter, sizeof(int), meshCounter, frameIndex);
        mConstBuffers[MODEL_INDEX_CB]->Bind(commandList, 3, meshCounter, frameIndex);

        engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->BindToGraphics(commandList, 16, materialHeapSlot);

        //DRAW THE MESH
            staticMeshComponent.mStaticMesh->DrawMesh();
            meshCounter++;
    }
    }
    
    //RENDER SKINNED MESHES

    commandList->SetPipelineState(mPBRSkinnedPipeline->GetPipeline().Get());

    {
        const auto view = world.GetRegistry().View<const SkinnedMeshComponent, const TransformComponent>();
        
        for (auto [entity, skinnedMeshComponent, transform] : view.each())
        {
            if (!skinnedMeshComponent.mSkinnedMesh)
            {
                continue;
            }

            //UPDATE AND BIND MATERIAL INFO
            InfoStruct::DXMaterialInfo materialInfo;
            if (skinnedMeshComponent.mMaterial != nullptr) {
                materialInfo.colorFactor = { skinnedMeshComponent.mMaterial->mBaseColorFactor.r,
                    skinnedMeshComponent.mMaterial->mBaseColorFactor.g,
                    skinnedMeshComponent.mMaterial->mBaseColorFactor.b,
                    0.f };
                materialInfo.emissiveFactor = { skinnedMeshComponent.mMaterial->mEmissiveFactor.r,
                    skinnedMeshComponent.mMaterial->mEmissiveFactor.g,
                    skinnedMeshComponent.mMaterial->mEmissiveFactor.b,
                    0.f };
                materialInfo.metallicFactor = skinnedMeshComponent.mMaterial->mMetallicFactor;
                materialInfo.roughnessFactor = skinnedMeshComponent.mMaterial->mRoughnessFactor;
                materialInfo.normalScale = skinnedMeshComponent.mMaterial->mNormalScale;

                materialInfo.useColorTex = skinnedMeshComponent.mMaterial->mBaseColorTexture != nullptr && skinnedMeshComponent.mMaterial->mBaseColorTexture->WasSendToGPU();
                materialInfo.useEmissiveTex = skinnedMeshComponent.mMaterial->mEmissiveTexture != nullptr && skinnedMeshComponent.mMaterial->mEmissiveTexture->WasSendToGPU();
                materialInfo.useMetallicRoughnessTex = skinnedMeshComponent.mMaterial->mMetallicRoughnessTexture != nullptr && skinnedMeshComponent.mMaterial->mMetallicRoughnessTexture->WasSendToGPU();
                materialInfo.useNormalTex = skinnedMeshComponent.mMaterial->mNormalTexture != nullptr && skinnedMeshComponent.mMaterial->mNormalTexture->WasSendToGPU();
                materialInfo.useOcclusionTex = skinnedMeshComponent.mMaterial->mOcclusionTexture != nullptr && skinnedMeshComponent.mMaterial->mOcclusionTexture->WasSendToGPU();

                //BIND TEXTURES
                if (materialInfo.useColorTex)
                {
                    skinnedMeshComponent.mMaterial->mBaseColorTexture->BindToGraphics(commandList, 5);
                }
                if (materialInfo.useEmissiveTex)
                {
                    skinnedMeshComponent.mMaterial->mEmissiveTexture->BindToGraphics(commandList, 6);
                }
                if (materialInfo.useMetallicRoughnessTex)
                {
                    skinnedMeshComponent.mMaterial->mMetallicRoughnessTexture->BindToGraphics(commandList, 7);
                }
                if (materialInfo.useNormalTex)
                {
                    skinnedMeshComponent.mMaterial->mNormalTexture->BindToGraphics(commandList, 8);
                }
                if (materialInfo.useOcclusionTex)
                {
                    skinnedMeshComponent.mMaterial->mOcclusionTexture->BindToGraphics(commandList, 9);
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
            materials[meshCounter] = materialInfo;

            mConstBuffers[MODEL_MATRIX_CB]->Bind(commandList, 2, meshCounter, frameIndex);

            mConstBuffers[FINAL_BONE_MATRIX_CB]->Bind(commandList, 5, meshCounter, frameIndex);

            //UPDATE AND BIND MATERIAL INFO
            mConstBuffers[MODEL_INDEX_CB]->Update(&meshCounter, sizeof(int), meshCounter, frameIndex);
            mConstBuffers[MODEL_INDEX_CB]->Bind(commandList, 3, meshCounter, frameIndex);

            engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->BindToGraphics(commandList, 16, materialHeapSlot);

            skinnedMeshComponent.mSkinnedMesh->DrawMesh();

            meshCounter++;
        }
    }

    D3D12_SUBRESOURCE_DATA data;
    data.pData = materials.data();
    data.RowPitch = sizeof(InfoStruct::DXMaterialInfo);
    data.SlicePitch = sizeof(InfoStruct::DXMaterialInfo) * (MAX_MESHES + 2);
    mStructuredBuffers[DXStructuredBuffers::MATERIAL_SB]->Update(commandList, data, D3D12_RESOURCE_STATE_GENERIC_READ, 0, 1);

    memset(materials.data(), 0, sizeof(InfoStruct::DXMaterialInfo) * materials.size());
}

Engine::MetaType Engine::Renderer::Reflect()
{
    return MetaType{ MetaType::T<Renderer>{}, "Renderer", MetaType::Base<System>{} };
}

void SendMaterialToGPUIfReady(const Engine::Material& mat)
{
    if (mat.mBaseColorTexture != nullptr
        && mat.mBaseColorTexture->IsReadyToSendToGPU())
    {
        mat.mBaseColorTexture->SendToGPU();
    }
    if (mat.mEmissiveTexture != nullptr
        && mat.mEmissiveTexture->IsReadyToSendToGPU())
    {
        mat.mEmissiveTexture->SendToGPU();
    }

    if (mat.mMetallicRoughnessTexture != nullptr
        && mat.mMetallicRoughnessTexture->IsReadyToSendToGPU())
    {
        mat.mMetallicRoughnessTexture->SendToGPU();
    }

    if (mat.mNormalTexture != nullptr
        && mat.mNormalTexture->IsReadyToSendToGPU())
    {
        mat.mNormalTexture->SendToGPU();
    }

    if (mat.mOcclusionTexture != nullptr
        && mat.mOcclusionTexture->IsReadyToSendToGPU())
    {
        mat.mOcclusionTexture->SendToGPU();
    }
}