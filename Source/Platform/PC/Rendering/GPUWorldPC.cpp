#include "Precomp.h"
#include "Platform/PC/Rendering/GPUWorldPC.h"

#include "World/World.h"
#include "World/WorldViewport.h"
#include "World/Registry.h"

#include "Assets/Material.h"
#include "Assets/Texture.h"
#include "Assets/StaticMesh.h"
#include "Assets/SkinnedMesh.h"

#include "Components/TransformComponent.h"
#include "Components/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkinnedMeshComponent.h"

#include "Platform/PC/Core/DevicePC.h"
#include "Platform/PC/Rendering/DX12Classes/DXConstBuffer.h"

Engine::DebugRenderingData::DebugRenderingData()
{
    Device& engineDevice = Device::Get();
    ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());
    ID3D12GraphicsCommandList4* uploadCmdList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetUploadCommandList());

    std::vector<glm::vec3> positions(2);
    positions[0] = glm::vec3(0.f, 0.f, 0.f);
    positions[1] = glm::vec3(1.f, 0.f, 0.f);

    engineDevice.StartUploadCommands();
    int vertexCount = 2;
    int vBufferSize = sizeof(float) * vertexCount * 3;

    mVertexBuffer = std::make_unique<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), CD3DX12_RESOURCE_DESC::Buffer(vBufferSize), nullptr, "Line Vertex resource buffer");

    D3D12_SUBRESOURCE_DATA vData = {};
    vData.pData = positions.data();
    vData.RowPitch = sizeof(float) * 3;
    vData.SlicePitch = vBufferSize;

    mVertexBuffer->CreateUploadBuffer(device, vBufferSize, 0);
    mVertexBuffer->Update(uploadCmdList, vData, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, 0, 1);

    mVertexBufferView.BufferLocation = mVertexBuffer->GetResource()->GetGPUVirtualAddress();
    mVertexBufferView.StrideInBytes = sizeof(float) * 3;
    mVertexBufferView.SizeInBytes = vBufferSize;

    mLineColorBuffer = std::make_unique<DXConstBuffer>(device, sizeof(glm::vec4), MAX_LINES, "Lines color const buffer", FRAME_BUFFER_COUNT);
    mLineMatrixBuffer = std::make_unique<DXConstBuffer>(device, sizeof(glm::mat4x4), MAX_LINES, "Lines matrix const buffer", FRAME_BUFFER_COUNT);

    engineDevice.SubmitUploadCommands();
}

Engine::DebugRenderingData::~DebugRenderingData() = default;

Engine::GPUWorld::GPUWorld(const World& world)
    :
    IGPUWorld::IGPUWorld(world)
{
    Device& engineDevice = Device::Get();
    ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());

    // Create constant buffers
    mConstBuffers[CAM_MATRIX_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::DXMatrixInfo), 1, "Matrix buffer default shader", FRAME_BUFFER_COUNT);
    mConstBuffers[LIGHT_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::DXLightInfo), 1, "Point light buffer", FRAME_BUFFER_COUNT);
    mConstBuffers[MODEL_INDEX_CB] = std::make_unique<DXConstBuffer>(device, sizeof(int), MAX_MESHES + 2, "Model index", FRAME_BUFFER_COUNT);
    mConstBuffers[MODEL_MATRIX_CB] = std::make_unique<DXConstBuffer>(device, sizeof(glm::mat4x4) * 2, MAX_MESHES, "Mesh matrix data", FRAME_BUFFER_COUNT);
    mConstBuffers[FINAL_BONE_MATRIX_CB] = std::make_unique<DXConstBuffer>(device, sizeof(glm::mat4x4) * MAX_BONES, MAX_SKINNED_MESHES, "Skinned Mesh Bone Matrices", FRAME_BUFFER_COUNT);

    // Create structured buffers
    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(InfoStruct::DXMaterialInfo) * (MAX_MESHES + 2), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mStructuredBuffers[InfoStruct::DXStructuredBuffers::MATERIAL_SB] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "MATERIAL STRUCTURED BUFFER");
    mStructuredBuffers[InfoStruct::DXStructuredBuffers::MATERIAL_SB]->CreateUploadBuffer(device, sizeof(InfoStruct::DXMaterialInfo) * (MAX_MESHES + 2), 0);

    D3D12_SHADER_RESOURCE_VIEW_DESC  srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.StructureByteStride = sizeof(InfoStruct::DXMaterialInfo);
    srvDesc.Buffer.NumElements = MAX_MESHES + 2;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    mMaterialHeapSlot = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateResource(mStructuredBuffers[InfoStruct::DXStructuredBuffers::MATERIAL_SB].get(), &srvDesc);
    mMaterials = std::vector<InfoStruct::DXMaterialInfo>(MAX_MESHES + 2);
}

Engine::GPUWorld::~GPUWorld() = default;

void Engine::GPUWorld::Update()
{
    Device& engineDevice = Device::Get();
    int frameIndex = engineDevice.GetFrameIndex();

    // Get main camera
    const auto optionalEntityCameraPair = mWorld.get().GetViewport().GetMainCamera();
    ASSERT_LOG(optionalEntityCameraPair.has_value(), "DX12 draw requests have been made, but they cannot be cleared as there is no camera to draw them to");

    // Update camera
    const auto camera = optionalEntityCameraPair->second;
    InfoStruct::DXMatrixInfo matrixInfo{};
    matrixInfo.pm = glm::transpose(camera.GetProjection());
    matrixInfo.vm = glm::transpose(camera.GetView());

    matrixInfo.ipm = glm::inverse(matrixInfo.pm);
    matrixInfo.ivm = glm::inverse(matrixInfo.vm);
    mConstBuffers[CAM_MATRIX_CB]->Update(&matrixInfo, sizeof(InfoStruct::DXMatrixInfo), 0, frameIndex);

    // Update lights
    const auto pointLightView = mWorld.get().GetRegistry().View<const PointLightComponent, const TransformComponent>();
    int pointLightCounter = 0;
    for (auto [entity, lightComponent, transform] : pointLightView.each()) 
    {
        if (pointLightCounter < MAX_LIGHTS) 
        {
            mLights.mPointLights[pointLightCounter].mPosition = glm::vec4(transform.GetWorldPosition(), 1.f);
            mLights.mPointLights[pointLightCounter].mColorAndIntensity = glm::vec4(lightComponent.mColor, lightComponent.mIntensity);
            mLights.mPointLights[pointLightCounter].mRadius = lightComponent.mRange;
        }
        else 
        {
            LOG(LogCore, Warning, ("Maximum (%i) of point lights has been surpassed.", MAX_LIGHTS));
            break;
        }

        pointLightCounter++;
    }

    const auto dirLightView = mWorld.get().GetRegistry().View<const DirectionalLightComponent, const TransformComponent>();
    int dirLightCounter = 0;
    for (auto [entity, lightComponent, transform] : dirLightView.each()) 
    {
        if (dirLightCounter < MAX_LIGHTS) 
        {
            glm::quat quatRotation = transform.GetLocalOrientation();
            glm::vec3 baseDir = glm::vec3(0, 0, 1);
            glm::vec3 lightDirection = quatRotation * baseDir;

            mLights.mDirLights[dirLightCounter].mDir = glm::vec4(lightDirection, 1.f);
            mLights.mDirLights[dirLightCounter].mColorAndIntensity = glm::vec4(lightComponent.mColor, lightComponent.mIntensity);
        }
        else 
        {
            LOG(LogCore, Warning, ("Maximum (%i) of directional lights has been surpassed.", MAX_LIGHTS));
            break;
        }

        dirLightCounter++;
    }

    mConstBuffers[LIGHT_CB]->Update(&mLights, sizeof(InfoStruct::DXLightInfo), 0, frameIndex);


    // Update materials
    // 
    // I'm not sure why, because I (Guus), know nothing of dx12, but dx12 does not like it
    // if we are sending textures to the GPU in the RENDERING pass. Soo my solution was to
    // do it here instead. The only downside is that we have to iterate over the static meshes
    // twice, but in entt this is surprisingly fast.
    int meshCounter = 0;

    {
        // We get transform to make sure the mesh count is correct, since the user can have a mesh without a transform
        const auto view = mWorld.get().GetRegistry().View<const StaticMeshComponent, const TransformComponent>();

        for (auto [entity, staticMeshComponent, transformComponent] : view.each())
        {
            if (!staticMeshComponent.mStaticMesh)
            {
                continue;
            }

            InfoStruct::DXMaterialInfo materialInfo{};
            if (staticMeshComponent.mMaterial != nullptr)
            {
                SendMaterialTexturesToGPU(*staticMeshComponent.mMaterial);

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
                materialInfo.useColorTex = staticMeshComponent.mMaterial->mBaseColorTexture != nullptr && staticMeshComponent.mMaterial->mBaseColorTexture->WasSentToGpu();
                materialInfo.useEmissiveTex = staticMeshComponent.mMaterial->mEmissiveTexture != nullptr && staticMeshComponent.mMaterial->mEmissiveTexture->WasSentToGpu();
                materialInfo.useMetallicRoughnessTex = staticMeshComponent.mMaterial->mMetallicRoughnessTexture != nullptr && staticMeshComponent.mMaterial->mMetallicRoughnessTexture->WasSentToGpu();
                materialInfo.useNormalTex = staticMeshComponent.mMaterial->mNormalTexture != nullptr && staticMeshComponent.mMaterial->mNormalTexture->WasSentToGpu();
                materialInfo.useOcclusionTex = staticMeshComponent.mMaterial->mOcclusionTexture != nullptr && staticMeshComponent.mMaterial->mOcclusionTexture->WasSentToGpu();
            }
            else 
            {
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

            mMaterials[meshCounter] = materialInfo;
            meshCounter++;
        }
    }

    {
        // We get transform to make sure the mesh count is correct, since the user can have a mesh without a transform
        const auto view = mWorld.get().GetRegistry().View<const SkinnedMeshComponent, const TransformComponent>();

        for (auto [entity, skinnedMeshComponent, transformComponent] : view.each())
        {
            if (!skinnedMeshComponent.mSkinnedMesh)
            {
                continue;
            }

            // Update material
            InfoStruct::DXMaterialInfo materialInfo{};
            if (skinnedMeshComponent.mMaterial != nullptr) 
            {
                SendMaterialTexturesToGPU(*skinnedMeshComponent.mMaterial);

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

                materialInfo.useColorTex = skinnedMeshComponent.mMaterial->mBaseColorTexture != nullptr && skinnedMeshComponent.mMaterial->mBaseColorTexture->WasSentToGpu();
                materialInfo.useEmissiveTex = skinnedMeshComponent.mMaterial->mEmissiveTexture != nullptr && skinnedMeshComponent.mMaterial->mEmissiveTexture->WasSentToGpu();
                materialInfo.useMetallicRoughnessTex = skinnedMeshComponent.mMaterial->mMetallicRoughnessTexture != nullptr && skinnedMeshComponent.mMaterial->mMetallicRoughnessTexture->WasSentToGpu();
                materialInfo.useNormalTex = skinnedMeshComponent.mMaterial->mNormalTexture != nullptr && skinnedMeshComponent.mMaterial->mNormalTexture->WasSentToGpu();
                materialInfo.useOcclusionTex = skinnedMeshComponent.mMaterial->mOcclusionTexture != nullptr && skinnedMeshComponent.mMaterial->mOcclusionTexture->WasSentToGpu();
            }
            else 
            {
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

            mMaterials[meshCounter] = materialInfo;
            meshCounter++;
        }
    }
}

void Engine::GPUWorld::UpdateMaterials()
{
    Device& engineDevice = Device::Get();
    ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());

    D3D12_SUBRESOURCE_DATA data{};
    data.pData = mMaterials.data();
    data.RowPitch = sizeof(InfoStruct::DXMaterialInfo);
    data.SlicePitch = sizeof(InfoStruct::DXMaterialInfo) * (MAX_MESHES + 2);
    mStructuredBuffers[InfoStruct::DXStructuredBuffers::MATERIAL_SB]->Update(commandList, data, D3D12_RESOURCE_STATE_GENERIC_READ, 0, 1);

    memset(mMaterials.data(), 0, sizeof(InfoStruct::DXMaterialInfo) * mMaterials.size());
}

void Engine::GPUWorld::SendMaterialTexturesToGPU(const Engine::Material& mat)
{
    if (mat.mBaseColorTexture != nullptr
        && mat.mBaseColorTexture->IsReadyToBeSentToGpu())
    {
        mat.mBaseColorTexture->SentToGPU();
    }

    if (mat.mEmissiveTexture != nullptr
        && mat.mEmissiveTexture->IsReadyToBeSentToGpu())
    {
        mat.mEmissiveTexture->SentToGPU();
    }

    if (mat.mMetallicRoughnessTexture != nullptr
        && mat.mMetallicRoughnessTexture->IsReadyToBeSentToGpu())
    {
        mat.mMetallicRoughnessTexture->SentToGPU();
    }

    if (mat.mNormalTexture != nullptr
        && mat.mNormalTexture->IsReadyToBeSentToGpu())
    {
        mat.mNormalTexture->SentToGPU();
    }

    if (mat.mOcclusionTexture != nullptr
        && mat.mOcclusionTexture->IsReadyToBeSentToGpu())
    {
        mat.mOcclusionTexture->SentToGPU();
    }
}