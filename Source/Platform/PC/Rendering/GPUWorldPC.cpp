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
#include "Rendering/GPUWorld.h"

#include "Platform/PC/Core/DevicePC.h"
#include "Platform/PC/Rendering/DX12Classes/DXConstBuffer.h"

Engine::DebugRenderingData::DebugRenderingData()
{
    Device& engineDevice = Device::Get();
    ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());

    uint bufferSize = sizeof(glm::vec3) * MAX_LINE_VERTICES;
    mVertexPositionBuffer = std::make_unique<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), 
        CD3DX12_RESOURCE_DESC::Buffer(bufferSize), nullptr, "Line vertex position buffer");
    mVertexPositionBuffer->CreateUploadBuffer(device, bufferSize, 0);

    mVertexPositionBufferView.BufferLocation = mVertexPositionBuffer->GetResource()->GetGPUVirtualAddress();
    mVertexPositionBufferView.StrideInBytes = sizeof(glm::vec3);
    mVertexPositionBufferView.SizeInBytes = bufferSize;

    bufferSize = sizeof(glm::vec4) * MAX_LINE_VERTICES;
    mVertexColorBuffer = std::make_unique<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), 
        CD3DX12_RESOURCE_DESC::Buffer(bufferSize), nullptr, "Line vertex color buffer");
    mVertexColorBuffer->CreateUploadBuffer(device, bufferSize, 0);

    mVertexColorBufferView.BufferLocation = mVertexColorBuffer->GetResource()->GetGPUVirtualAddress();
    mVertexColorBufferView.StrideInBytes = sizeof(glm::vec4);
    mVertexColorBufferView.SizeInBytes = bufferSize;

    mPositions.resize(MAX_LINE_VERTICES);
    mColors.resize(MAX_LINE_VERTICES);
}

Engine::DebugRenderingData::~DebugRenderingData() = default;

Engine::UIRenderingData::UIRenderingData()
{
    Device& engineDevice = Device::Get();
    ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());
    ID3D12GraphicsCommandList4* uploadCmdList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetUploadCommandList());
    engineDevice.StartUploadCommands();

    std::vector<glm::vec3> positions =
    {
        glm::vec3(-0.5f, 0.5f, 0.0f),  // Top Left
        glm::vec3(0.5f, 0.5f, 0.0f),   // Top Right
        glm::vec3(-0.5f, -0.5f, 0.0f), // Bottom Left
        glm::vec3(0.5f, -0.5f, 0.0f),  // Bottom Right
    };
    std::vector<glm::vec2> uvs = 
    {
        glm::vec2(0.f),
        glm::vec2(1.f, 0.f),
        glm::vec2(0.f, 1.f),
        glm::vec2(1.f, 1.f)
    };
    std::vector<uint32> indices = { 0, 1, 2, 3, 2, 1 };
    int vBufferSize = sizeof(glm::vec3) * static_cast<int>(positions.size());
    int tBufferSize = sizeof(glm::vec2) * static_cast<int>(uvs.size());
    int iBufferSize = sizeof(uint32) * static_cast<int>(indices.size());

    mQuadVResource = std::make_unique<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), CD3DX12_RESOURCE_DESC::Buffer(vBufferSize), nullptr, "UI Vertex resource buffer");
    mQuadUVResource = std::make_unique<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), CD3DX12_RESOURCE_DESC::Buffer(tBufferSize), nullptr, "UI UV resource buffer");
    mIndicesResource = std::make_unique<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), CD3DX12_RESOURCE_DESC::Buffer(iBufferSize), nullptr, "UI indices resource buffer");

    mQuadVResource->CreateUploadBuffer(device, vBufferSize, 0);
    mQuadUVResource->CreateUploadBuffer(device, tBufferSize, 0);
    mIndicesResource->CreateUploadBuffer(device, iBufferSize, 0);

    D3D12_SUBRESOURCE_DATA vData = {};
    vData.pData = positions.data();
    vData.RowPitch = sizeof(float) * 3;
    vData.SlicePitch = vBufferSize;

    D3D12_SUBRESOURCE_DATA uData = {};
    uData.pData = uvs.data();
    uData.RowPitch = sizeof(float) * 2;
    uData.SlicePitch = tBufferSize;

    D3D12_SUBRESOURCE_DATA iData = {};
    iData.pData = indices.data();
    iData.RowPitch = iBufferSize;
    iData.SlicePitch = iBufferSize;

    mQuadVResource->Update(uploadCmdList, vData, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, 0, 1);
    mQuadUVResource->Update(uploadCmdList, uData, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, 0, 1);
    mIndicesResource->Update(uploadCmdList, iData, D3D12_RESOURCE_STATE_INDEX_BUFFER, 0, 1);

    mVertexBufferView.BufferLocation = mQuadVResource->GetResource()->GetGPUVirtualAddress();
    mVertexBufferView.StrideInBytes = sizeof(float) * 3;
    mVertexBufferView.SizeInBytes = vBufferSize;

    mTexCoordBufferView.BufferLocation = mQuadUVResource->GetResource()->GetGPUVirtualAddress();
    mTexCoordBufferView.StrideInBytes = sizeof(float) * 2;
    mTexCoordBufferView.SizeInBytes = tBufferSize;

    mIndexBufferView.BufferLocation = mIndicesResource->GetResource()->GetGPUVirtualAddress();
    mIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
    mIndexBufferView.SizeInBytes = iBufferSize;

    mModelMatBuffer = std::make_unique<DXConstBuffer>(device, sizeof(glm::mat4x4) * 2, MAX_MESHES, "UI Mesh matrix data", FRAME_BUFFER_COUNT);
    mColorBuffer = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::ColorInfo), MAX_MESHES, "UI Mesh color data", FRAME_BUFFER_COUNT);
    engineDevice.SubmitUploadCommands();
}

Engine::GPUWorld::GPUWorld(const World& world)
    :
    IGPUWorld::IGPUWorld(world)
{
    Device& engineDevice = Device::Get();
    ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());

    // Create constant buffers
    //TODO: Increase number of buffer capability depending on number of directional lights
    mConstBuffers[CAM_MATRIX_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::DXMatrixInfo), 2001, "Matrix buffer default shader", FRAME_BUFFER_COUNT);
    mConstBuffers[LIGHT_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::DXLightInfo), 1, "Point light buffer", FRAME_BUFFER_COUNT);
    mConstBuffers[MATERIAL_INFO_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::DXMaterialInfo), MAX_MESHES + 2, "Model material info", FRAME_BUFFER_COUNT);
    mConstBuffers[MODEL_MATRIX_CB] = std::make_unique<DXConstBuffer>(device, sizeof(glm::mat4x4) * 2, MAX_MESHES, "Mesh matrix data", FRAME_BUFFER_COUNT);
    mConstBuffers[FINAL_BONE_MATRIX_CB] = std::make_unique<DXConstBuffer>(device, sizeof(glm::mat4x4) * MAX_BONES, MAX_SKINNED_MESHES, "Skinned mesh bone matrices", FRAME_BUFFER_COUNT);
    mConstBuffers[COLOR_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::DXColorMultiplierInfo), MAX_MESHES, "Color multiplier", FRAME_BUFFER_COUNT);
    mConstBuffers[UI_MODEL_MAT_CB] = std::make_unique<DXConstBuffer>(device, sizeof(glm::mat4x4) * 2, MAX_MESHES, "UI MODEL MATRICES", FRAME_BUFFER_COUNT);
    mShadowMaps.resize(20);
    InitializeShadowMaps();
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
    glm::vec3 cameraPos = glm::transpose(matrixInfo.ivm)[3];


    float nearPlane = camera.mNear;
    float farPlane = camera.mFar;

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
            float extent = lightComponent.mExtent;

            InfoStruct::DXMatrixInfo lightCameraMap;
            glm::vec3 lightForward = transform.GetWorldForward();
            glm::mat4x4 projection = glm::orthoLH_ZO(extent * -0.5f, extent * 0.5f, extent * 0.5f, extent * -0.5f, nearPlane, farPlane);
           // glm::mat4x4 projection = glm::orthoLH_ZO(-10.0f, 10.0f, -10.0f, 10.0f, nearPlane, farPlane);
            glm::mat4x4 view = glm::inverse(transform.GetWorldMatrix());

            lightCameraMap.pm = glm::transpose(projection);
            lightCameraMap.vm = glm::transpose(view);

            mLights.mDirLights[dirLightCounter].lightMat = glm::inverse(glm::transpose(view * projection));

            mConstBuffers[CAM_MATRIX_CB]->Update(&lightCameraMap, sizeof(InfoStruct::DXMatrixInfo), dirLightCounter+1, frameIndex);

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
                materialInfo.normalScale = 1.f;
                materialInfo.useColorTex = false;
                materialInfo.useEmissiveTex = false;
                materialInfo.useMetallicRoughnessTex = false;
                materialInfo.useNormalTex = false;
                materialInfo.useOcclusionTex = false;

            }

            mConstBuffers[MATERIAL_INFO_CB]->Update(&materialInfo, sizeof(InfoStruct::DXMaterialInfo), meshCounter, frameIndex);
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
                materialInfo.normalScale = 1.f;
                materialInfo.useColorTex = false;
                materialInfo.useEmissiveTex = false;
                materialInfo.useMetallicRoughnessTex = false;
                materialInfo.useNormalTex = false;
                materialInfo.useOcclusionTex = false;
            }

            mConstBuffers[MATERIAL_INFO_CB]->Update(&materialInfo, sizeof(InfoStruct::DXMaterialInfo), meshCounter, frameIndex);
            meshCounter++;
        }
    }
}

void Engine::GPUWorld::SendMaterialTexturesToGPU(const Engine::Material& mat)
{
    if (mat.mBaseColorTexture != nullptr
        && mat.mBaseColorTexture->IsReadyToBeSentToGpu())
    {
        mat.mBaseColorTexture->SendToGPU();
    }

    if (mat.mEmissiveTexture != nullptr
        && mat.mEmissiveTexture->IsReadyToBeSentToGpu())
    {
        mat.mEmissiveTexture->SendToGPU();
    }

    if (mat.mMetallicRoughnessTexture != nullptr
        && mat.mMetallicRoughnessTexture->IsReadyToBeSentToGpu())
    {
        mat.mMetallicRoughnessTexture->SendToGPU();
    }

    if (mat.mNormalTexture != nullptr
        && mat.mNormalTexture->IsReadyToBeSentToGpu())
    {
        mat.mNormalTexture->SendToGPU();
    }

    if (mat.mOcclusionTexture != nullptr
        && mat.mOcclusionTexture->IsReadyToBeSentToGpu())
    {
        mat.mOcclusionTexture->SendToGPU();
    }
}

void Engine::GPUWorld::InitializeShadowMaps()
{
    for (int i = 0; i < mShadowMaps.size(); i++) {
        std::unique_ptr<InfoStruct::DXShadowMapInfo>& shadowMap = mShadowMaps[i];
        shadowMap = std::make_unique<InfoStruct::DXShadowMapInfo>();
        Device& engineDevice = Device::Get();
        ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());

        D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
        depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
        depthOptimizedClearValue.DepthStencil.Stencil = 0;
        auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, 2048, 2048, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
        shadowMap->mDepthResource = std::make_unique<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), resourceDesc, &depthOptimizedClearValue, "DIRECTIONAL LIGHT DEPTH STENCIL");

        D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
        depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
        depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;
        shadowMap->mDepthHandle = engineDevice.GetDescriptorHeap(DEPTH_HEAP)->AllocateDepthStencil(shadowMap->mDepthResource.get(), &depthStencilDesc);

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Use the format that matches your RTV format.
        clearValue.Color[0] = 0.f; // Red component
        clearValue.Color[1] = 0.f; // Green component
        clearValue.Color[2] = 0.f; // Blue component
        clearValue.Color[3] = 0.f; // Alpha component
        resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 2048, 2048, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
        shadowMap->mRenderTarget = std::make_unique<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), resourceDesc, &clearValue, "DIRECTIONAL LIGHT RENDER TARGET");

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        rtvDesc.Texture2D.MipSlice = 0;
        shadowMap-> mRTHandle = engineDevice.GetDescriptorHeap(RT_HEAP)->AllocateRenderTarget(shadowMap->mRenderTarget.get(), &rtvDesc);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        shadowMap->mDepthSRVHandle = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateResource(shadowMap->mDepthResource.get(), &srvDesc);

        shadowMap->mViewport.Width = static_cast<FLOAT>(2048);
        shadowMap->mViewport.Height = static_cast<FLOAT>(2048);
        shadowMap->mViewport.TopLeftX = 0;
        shadowMap->mViewport.TopLeftY = 0;
        shadowMap->mViewport.MinDepth = 0.0f;
        shadowMap->mViewport.MaxDepth = 1.0f;

        shadowMap->mScissorRect.left = 0;
        shadowMap->mScissorRect.top = 0;
        shadowMap->mScissorRect.right = static_cast<LONG>(shadowMap->mViewport.Width);
        shadowMap->mScissorRect.bottom = static_cast<LONG>(shadowMap->mViewport.Height);

    }

}
