#include "Precomp.h"
#include "Platform/PC/Core/DevicePC.h"
#include "Core/FileIO.h"

#pragma warning(push)
#pragma warning(disable: 4189)
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_dx12.h"
#include "imgui/ImGuizmo.h"
#pragma warning(pop)

#include "Platform/PC/Rendering/DX12Classes/DXDescHeap.h"

Engine::Device::Device()
{
    InitializeWindow();
	InitializeDevice();
}

void Engine::Device::InitializeWindow()
{
    LOG(LogCore, Verbose, "Initializing GLFW");

    if (!glfwInit())
    {
        LOG(LogCore, Fatal, "GLFW could not be initialized");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    mMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(mMonitor);

    auto maxScreenWidth = mode->width;
    auto maxScreenHeight = mode->height;

    mViewport.Width = 1920;
    mViewport.Height = 1080;
    mPreviousWidth = 1920;
    mPreviousHeight = 1080;

    mFullscreen = false;

    if (mFullscreen)
    {
        mViewport.Width = static_cast<FLOAT>(maxScreenWidth);
        mViewport.Height = static_cast<FLOAT>(maxScreenHeight);
        mWindow = glfwCreateWindow(static_cast<int>(mViewport.Width), static_cast<int>(mViewport.Height), "General engine", mMonitor, nullptr);
    }
    else
    {
        glfwWindowHint(GLFW_RESIZABLE, 1);
        mWindow = glfwCreateWindow(static_cast<int>(mViewport.Width), static_cast<int>(mViewport.Height), "General engine", nullptr, nullptr);
    }

    if (mWindow == nullptr)
    {
        LOG(LogCore, Fatal, "GLFW window could not be created");
    }

    glfwMakeContextCurrent(mWindow);
    glfwShowWindow(mWindow);
    mIsWindowOpen = true;
}

void Engine::Device::InitializeDevice()
{
    //DEBUG LAYERS
#if defined(_DEBUG)
    Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
    if (FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)))) {
        LOG(LogCore, Fatal, "Failed to get debug interface");
        ABORT;
    }
    debugInterface->EnableDebugLayer();
#endif

    mViewport.TopLeftX = 0;
    mViewport.TopLeftY = 0;
    mViewport.MinDepth = 0.0f;
    mViewport.MaxDepth = 1.0f;

    mScissorRect.left = 0;
    mScissorRect.top = 0;
    mScissorRect.right = static_cast<LONG>(mViewport.Width);
    mScissorRect.bottom = static_cast<LONG>(mViewport.Height);

    ComPtr<IDXGIFactory4> dxgiFactory;
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
    if (FAILED(hr)) 
    {
        LOG(LogCore, Fatal, "Failed to create dxgi factory");
    }

    ComPtr<IDXGIAdapter1> adapter;
    int adapterIndex = 0;
    bool adapterFound = false;
    while (dxgiFactory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            adapterIndex++;
            continue;
        }

        hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr);
        if (SUCCEEDED(hr)) {

            hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mDevice));
            if (FAILED(hr)) {
                LOG(LogCore, Fatal, "Failed to create render device");
            }

            D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
            hr = mDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5,
                &options5, sizeof(options5));

            if (FAILED(hr)) 
            {
                LOG(LogCore, Fatal, "Failed to pick adaptor");
            }
            if (options5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED) {
                adapterFound = false;
                break;
            }
        }

        adapterIndex++;
    }

    //CREATE COMMAND QUEUE
    D3D12_COMMAND_QUEUE_DESC cqDesc = {};
    hr = mDevice->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&mCommandQueue));
    if (FAILED(hr)) {
        LOG(LogCore, Fatal, "Failed to create command queue");
    }

    hr = mDevice->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&mUploadCommandQueue));
    if (FAILED(hr)) 
    {
        LOG(LogCore, Fatal, "Failed to create upload command queue");
    }

    //CREATE COMMAND ALLOCATOR
    for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
    {
        hr = mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator[i]));
        if (FAILED(hr)) 
        {
            LOG(LogCore, Fatal, "Failed to create command allocator");
        }
    }

    hr = mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mUploadCommandAllocator));
    if (FAILED(hr)) 
    {
        LOG(LogCore, Fatal, "Failed to create upload command allocator");
    }

    //CREATE COMMAND LIST
    hr = mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator[0].Get(), NULL, IID_PPV_ARGS(&mCommandList));
    if (FAILED(hr))
    {
        LOG(LogCore, Fatal, "Failed to create command list");
    }
    mCommandList->SetName(L"Main command list");

    hr = mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mUploadCommandAllocator.Get(), NULL, IID_PPV_ARGS(&mUploadCommandList));
    if (FAILED(hr))
    {
        LOG(LogCore, Fatal, "Failed to create upload command list");
    }
    mUploadCommandList->SetName(L"Upload command list");
    mUploadCommandList->Close();

    //CREATE FENCES
    for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
    {
        hr = mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence[i]));
        if (FAILED(hr)) 
        {
            LOG(LogCore, Fatal, "Failed to create fence");
        }

        mFenceValue[i] = 0; // set the initial fence value to 0
    }

    hr = mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mUploadFence));
    if (FAILED(hr)) 
    {
        LOG(LogCore, Fatal, "Failed to upload create fence");
    }

    mUploadFenceValue = 0; // set the initial fence value to 0


    //CREATE FENCE EVENT
    mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (mFenceEvent == nullptr) 
    {
        LOG(LogCore, Fatal, "Failed to create fence event");
    }

    mUploadFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (mUploadFenceEvent == nullptr) 
    {
        LOG(LogCore, Fatal, "Failed to create upload fence event");
    }


    //CREATE SWAPCHAIN
    DXGI_MODE_DESC backBufferDesc = {};
    backBufferDesc.Width = static_cast<UINT>(mViewport.Width);
    backBufferDesc.Height = static_cast<UINT>(mViewport.Height);
    backBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    DXGI_SAMPLE_DESC sampleDesc = {};
    sampleDesc.Count = 1;
    sampleDesc.Quality = 0;

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = FRAME_BUFFER_COUNT;
    swapChainDesc.BufferDesc = backBufferDesc;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.OutputWindow = reinterpret_cast<HWND>(glfwGetWin32Window(mWindow));
    swapChainDesc.SampleDesc = sampleDesc;
    swapChainDesc.Windowed = true;

    IDXGISwapChain* tempSwapChain;
    hr = dxgiFactory->CreateSwapChain(mCommandQueue.Get(), &swapChainDesc, &tempSwapChain);
    if (FAILED(hr)) 
    {
        LOG(LogCore, Fatal, "Failed to create swap chain");
    }
    mSwapChain = static_cast<IDXGISwapChain3*>(tempSwapChain);

    //CREATE DESCRIPTOR HEAPS
    mDescriptorHeaps[RT_HEAP] = DXDescHeap::Construct(mDevice, 200, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, L"MAIN RENDER TARGETS HEAP");
    mDescriptorHeaps[DEPTH_HEAP] = DXDescHeap::Construct(mDevice, 200, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, L"DEPTH DESCRIPTOR HEAP");
    mDescriptorHeaps[RESOURCE_HEAP] = DXDescHeap::Construct(mDevice, 5000, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, L"RESOURCE HEAP", D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
    mDescriptorHeaps[SAMPLER_HEAP] = DXDescHeap::Construct(mDevice, 200, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, L"SAMPLER HEAP", D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

    //CREATE RENDER TARGETS
    for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
    {
        mResources[i] = std::make_unique<DXResource>();
        ComPtr<ID3D12Resource> res;
        hr = mSwapChain->GetBuffer(i, IID_PPV_ARGS(&res));
        if (FAILED(hr)) 
        {
            LOG(LogCore, Fatal, "Failed to get swapchain buffer");
        }
        mResources[i]->SetResource(res);
        mRenderTargetHandles[i] = mDescriptorHeaps[RT_HEAP]->AllocateRenderTarget(mResources[i].get(), mDevice.Get(), nullptr);
        mResources[i]->GetResource()->SetName(L"RENDER TARGET");
    }

    //CREATE ROOT SIGNATURE
    mSignature = std::make_unique<DXSignature>(15);
    mSignature->AddCBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX);//0
    mSignature->AddCBuffer(1, D3D12_SHADER_VISIBILITY_PIXEL);//1
    mSignature->AddCBuffer(2, D3D12_SHADER_VISIBILITY_VERTEX);//2
    mSignature->AddCBuffer(3, D3D12_SHADER_VISIBILITY_PIXEL);//3
    mSignature->AddCBuffer(4, D3D12_SHADER_VISIBILITY_PIXEL);//4
    mSignature->AddCBuffer(5, D3D12_SHADER_VISIBILITY_VERTEX);//5

    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);//6
    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);//7
    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);//8
    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3);//9
    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4);//10

    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 1);//11
    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 2);//12
    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 3);//13
    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 4);//14

    mSignature->AddTable(D3D12_SHADER_VISIBILITY_VERTEX, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5);//15
    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 6);//16
    mSignature->AddCBuffer(6, D3D12_SHADER_VISIBILITY_PIXEL);//17
    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 7);//18
    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 8);//19
    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 9);//20
    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 10);//21
    mSignature->AddCBuffer(5, D3D12_SHADER_VISIBILITY_PIXEL);//22

    mSignature->AddSampler(0, D3D12_SHADER_VISIBILITY_PIXEL, D3D12_TEXTURE_ADDRESS_MODE_WRAP);//18
    mSignature->CreateSignature(mDevice, L"MAIN ROOT SIGNATURE");

    //COMPUTE ROOT SIGNATURE
    //CREATE COMPUTE ROOT SIGNATURE
    mComputeSignature = std::make_unique<DXSignature>(8);
    mComputeSignature->AddCBuffer(0, D3D12_SHADER_VISIBILITY_ALL); // Cluster info 0
    mComputeSignature->AddCBuffer(1, D3D12_SHADER_VISIBILITY_ALL); // Camera info 1
    mComputeSignature->AddCBuffer(2, D3D12_SHADER_VISIBILITY_ALL); // Cluster camera info 2
    mComputeSignature->AddCBuffer(3, D3D12_SHADER_VISIBILITY_ALL); // Light info 3
    mComputeSignature->AddCBuffer(4, D3D12_SHADER_VISIBILITY_ALL); // Model matrices 4
    mComputeSignature->AddCBuffer(5, D3D12_SHADER_VISIBILITY_ALL); // Pixel color for cluster culling 5
    mComputeSignature->AddTable(D3D12_SHADER_VISIBILITY_ALL, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0); //6
    mComputeSignature->AddTable(D3D12_SHADER_VISIBILITY_ALL, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1); //7
    mComputeSignature->AddTable(D3D12_SHADER_VISIBILITY_ALL, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 2); //8
    mComputeSignature->AddTable(D3D12_SHADER_VISIBILITY_ALL, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 3); //9
    mComputeSignature->AddTable(D3D12_SHADER_VISIBILITY_ALL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); //10
    mComputeSignature->AddTable(D3D12_SHADER_VISIBILITY_ALL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1); //11
    mComputeSignature->AddTable(D3D12_SHADER_VISIBILITY_ALL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2); //12
    mComputeSignature->AddTable(D3D12_SHADER_VISIBILITY_ALL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3); //13
    mComputeSignature->AddSampler(0, D3D12_SHADER_VISIBILITY_ALL, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_FILTER_MIN_MAG_MIP_LINEAR);//14
    mComputeSignature->CreateSignature(mDevice, L"COMPUTE ROOT SIGNATURE");

    //CREATE DEPTH STENCIL
    D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
    depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
    depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

    D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
    depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
    depthOptimizedClearValue.DepthStencil.Stencil = 0;

    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(mDepthFormat, static_cast<UINT>(mViewport.Width), static_cast<UINT>(mViewport.Height), 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    mResources[DEPTH_STENCIL_RSC] = std::make_unique<DXResource>(mDevice, heapProperties, resourceDesc, &depthOptimizedClearValue, "Depth/Stencil Resource");
    mResources[DEPTH_STENCIL_RSC]->ChangeState(mCommandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    mDepthHandle = mDescriptorHeaps[DEPTH_HEAP]->AllocateDepthStencil(mResources[DEPTH_STENCIL_RSC].get(), mDevice.Get(), &depthStencilDesc);

    FileIO& fileIO = FileIO::Get();
    std::string shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/MipmapGen.hlsl");
    mGenMipmapsPipeline = std::make_unique<DXPipeline>();
    ComPtr<ID3DBlob> cs = DXPipeline::ShaderToBlob(shaderPath.c_str(), "cs_5_0");
    mGenMipmapsPipeline = std::make_unique<DXPipeline>();
    mGenMipmapsPipeline->SetComputeShader(cs->GetBufferPointer(), cs->GetBufferSize());
    mGenMipmapsPipeline->CreatePipeline(mDevice, mComputeSignature.get(), L"GENERATE MIPMAPS COMPUTE SHADER");

    SubmitCommands();
}

void Engine::Device::WaitForFence(ComPtr<ID3D12Fence> fence, UINT64& fenceValue, HANDLE& fenceEvent)
{
    UINT64 completedValue = fence->GetCompletedValue();
    if (completedValue < fenceValue)
    {
        if (FAILED(fence->SetEventOnCompletion(fenceValue, fenceEvent))) 
        {
            LOG(LogCore, Fatal, "Failed to set fence event on completion.");
        }
        WaitForSingleObject(fenceEvent, INFINITE);
    }
    fenceValue++;
}

void Engine::Device::UpdateRenderTarget()
{
    if (mViewport.Width <= 0 || mViewport.Height <= 0)
        return;

    mResources[0] = nullptr;
    mResources[1] = nullptr;
    mResources[DEPTH_STENCIL_RSC] = nullptr;

    HRESULT hr = mSwapChain->ResizeBuffers(FRAME_BUFFER_COUNT, static_cast<UINT>(mViewport.Width),  static_cast<UINT>(mViewport.Height), DXGI_FORMAT_R8G8B8A8_UNORM, 0);
    if (FAILED(hr)) 
    {
        LOG(LogCore, Fatal, "Failed to resize framebuffer");
    }

    for (int i = 0; i < FRAME_BUFFER_COUNT; i++) {
        mResources[i] = std::make_unique<DXResource>();
        ComPtr<ID3D12Resource> res;
        hr = mSwapChain->GetBuffer(i, IID_PPV_ARGS(&res));
        if (FAILED(hr)) 
        {
            LOG(LogCore, Fatal, "Failed to get swapchain buffer");
        }
        mResources[i]->SetResource(res);
        mRenderTargetHandles[i] = mDescriptorHeaps[RT_HEAP]->AllocateRenderTarget(mResources[i].get(), mDevice.Get(), nullptr);
        mResources[i]->GetResource()->SetName(L"RENDER TARGET");
    }

    D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
    depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

    D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
    depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
    depthOptimizedClearValue.DepthStencil.Stencil = 0;

    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(mDepthFormat, static_cast<UINT>(mViewport.Width), static_cast<UINT>(mViewport.Height), 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    mResources[DEPTH_STENCIL_RSC] = std::make_unique<DXResource>(mDevice, heapProperties, resourceDesc, &depthOptimizedClearValue, "Depth/Stencil Resource");
    mResources[DEPTH_STENCIL_RSC]->ChangeState(mCommandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    mDepthHandle = mDescriptorHeaps[DEPTH_HEAP]->AllocateDepthStencil(mResources[DEPTH_STENCIL_RSC].get(), mDevice.Get(), &depthStencilDesc);

    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();
}

void Engine::Device::NewFrame() {

    int width, height;
    mIsWindowOpen = !glfwWindowShouldClose(mWindow);

    glfwGetWindowSize(mWindow, &width, &height);
    if (width != mPreviousWidth || height != mPreviousHeight) {
        mViewport.Width = static_cast<float>(width);
        mViewport.Height = static_cast<float>(height);
        mPreviousWidth = width;
        mPreviousHeight = height;
        mScissorRect.right = static_cast<LONG>(mViewport.Width);
        mScissorRect.bottom = static_cast<LONG>(mViewport.Height);
        mUpdateWindow = true;
    }

#ifdef EDITOR
    ImGui::GetIO().DisplaySize.x = mViewport.Width;
    ImGui::GetIO().DisplaySize.y = mViewport.Height;
#endif // EDITOR

    StartRecordingCommands();

#ifdef EDITOR
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
#endif // EDITOR

    mCommandList->RSSetViewports(1, &mViewport); 
    mCommandList->RSSetScissorRects(1, &mScissorRect); 
    mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); 

    glm::vec4 clearColor(0.329f, 0.329f, 0.329f, 1.f);
    mResources[mFrameIndex]->ChangeState(mCommandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
    mDescriptorHeaps[RT_HEAP]->BindRenderTargets(mCommandList, &mRenderTargetHandles[mFrameIndex], mDepthHandle);
    mDescriptorHeaps[RT_HEAP]->ClearRenderTarget(mCommandList, mRenderTargetHandles[mFrameIndex], &clearColor[0]);
    mDescriptorHeaps[DEPTH_HEAP]->ClearDepthStencil(mCommandList, mDepthHandle);

}

void Engine::Device::EndFrame()
{   
    mDescriptorHeaps[RT_HEAP]->BindRenderTargets(mCommandList, &mRenderTargetHandles[mFrameIndex], mDepthHandle);

    auto* desc_ptr = mDescriptorHeaps[RESOURCE_HEAP]->Get();
    mCommandList->SetDescriptorHeaps(1, &desc_ptr);

#ifdef EDITOR
    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), mCommandList.Get());

    // Update and Render additional Platform Windows 
    // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere. 
    //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly) 
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
#endif // EDITOR

    glfwSwapBuffers(mWindow);
    mResources[mFrameIndex]->ChangeState(mCommandList, D3D12_RESOURCE_STATE_PRESENT);

    SubmitCommands();

    if (FAILED(mSwapChain->Present(0, 0))) 
    {
        LOG(LogCore, Fatal, "Failded to present");
    }

#ifdef EDITOR
    ImGui::EndFrame();
#endif // EDITOR

    WaitForFence(mFence[mFrameIndex], mFenceValue[mFrameIndex], mFenceEvent);

    if (mUpdateWindow)
    {
        mResources[0] = nullptr;
        mResources[1] = nullptr;
        mResources[DEPTH_STENCIL_RSC] = nullptr;
    }

    mResourcesToDeallocate.clear();

    if (mUpdateWindow)
    {
        int otherFrameIndex = mFrameIndex == 0 ? 1 : 0;
        mFenceValue[otherFrameIndex]++;
        mCommandQueue->Signal(mFence[otherFrameIndex].Get(), mFenceValue[otherFrameIndex]);
        WaitForFence(mFence[otherFrameIndex], mFenceValue[otherFrameIndex], mFenceEvent);
        StartRecordingCommands();
        UpdateRenderTarget();
        SubmitCommands();
        WaitForFence(mFence[mFrameIndex], mFenceValue[mFrameIndex], mFenceEvent);
        mUpdateWindow = false;
    }
}

void Engine::Device::SubmitCommands()
{
    //CLOSE COMMAND LIST
    mCommandList->Close();
    ID3D12CommandList* ppCommandLists[] = { mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    mFenceValue[mFrameIndex]++;
    HRESULT hr = mCommandQueue->Signal(mFence[mFrameIndex].Get(), mFenceValue[mFrameIndex]);

    if (FAILED(hr)) 
    {
        LOG(LogCore, Fatal, "Failed to signal fence");
    }

}

void Engine::Device::WaitForFence()
{
    SubmitCommands();
    WaitForFence(mFence[mFrameIndex], mFenceValue[mFrameIndex], mFenceEvent);
    StartRecordingCommands();
}

void Engine::Device::StartRecordingCommands()
{
    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

    if (FAILED(mCommandAllocator[mFrameIndex]->Reset())) 
    {
        LOG(LogCore, Fatal, "Failed to reset command allocator");
    }

    if (FAILED(mCommandList->Reset(mCommandAllocator[mFrameIndex].Get(), nullptr))) 
    {
        LOG(LogCore, Fatal, "Failed to reset command list");
    }
}

void Engine::Device::StartUploadCommands()
{

    if (FAILED(mUploadCommandAllocator->Reset())) 
    {
        LOG(LogCore, Fatal, "Failed to reset upload command allocator");
    }

    if (FAILED(mUploadCommandList->Reset(mUploadCommandAllocator.Get(), nullptr)))
    {
        LOG(LogCore, Fatal, "Failed to reset upload command list");
    }

}

void Engine::Device::SubmitUploadCommands()
{
    //CLOSE COMMAND LIST
    mUploadCommandList->Close();
    ID3D12CommandList* ppCommandLists[] = { mUploadCommandList.Get()};
    mUploadCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    mUploadFenceValue++;
    HRESULT hr = mUploadCommandQueue->Signal(mUploadFence.Get(), mUploadFenceValue);

    WaitForFence(mUploadFence, mUploadFenceValue, mUploadFenceEvent);
    
    if (FAILED(hr)) 
    {
        LOG(LogCore, Fatal, "Failed to signal upload fence");
    }

    WaitForFence(mUploadFence, mUploadFenceValue, mUploadFenceEvent);
}

void Engine::Device::AddToDeallocation(ComPtr<ID3D12Resource>&& res)
{
    mResourcesToDeallocate.emplace_back(std::move(res));
}

#ifdef EDITOR
void Engine::Device::CreateImguiContext()
{
    LOG(LogCore, Verbose, "Creating imgui context");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;
    ImGui::GetIO().ConfigViewportsNoDecoration = false;
    ImGui::GetIO().DisplaySize.x = mViewport.Width;
    ImGui::GetIO().DisplaySize.y = mViewport.Height;

    ImGui_ImplDX12_Init(mDevice.Get(), FRAME_BUFFER_COUNT,
        DXGI_FORMAT_R8G8B8A8_UNORM, mDescriptorHeaps[RESOURCE_HEAP]->Get(),
        mDescriptorHeaps[RESOURCE_HEAP]->Get()->GetCPUDescriptorHandleForHeapStart(),
        mDescriptorHeaps[RESOURCE_HEAP]->Get()->GetGPUDescriptorHandleForHeapStart());
    ImGui_ImplGlfw_InitForOther(mWindow, true);
    ImGui_ImplGlfw_SetCallbacksChainForAllWindows(true);

    ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());

    std::filesystem::create_directories("Intermediate/Layout");
    ImGui::GetIO().IniFilename = "Intermediate/Layout/imgui.ini";
}
#endif
