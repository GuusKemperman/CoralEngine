#include "Precomp.h"
#include "Platform/PC/Core/DevicePC.h"

#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_dx12.h"
#include "imgui/ImGuizmo.h"

#include "Platform/PC/Rendering/DX12Classes/DXDescHeap.h"
#include "Platform/PC/Rendering/DX12Classes/DXResource.h"

Engine::Device::Device() {

    InitializeWindow();
    InitializeDevice();
    InitializeImGui();
}

void Engine::Device::InitializeWindow()
{
    LOG(LogCore, Message, "Initializing GLFW");
    if (!glfwInit())
    {
        LOG(LogCore, Fatal, "GLFW could not be initialized");
        assert(false);
        exit(EXIT_FAILURE);
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

    mFullscreen = false;

    if (mFullscreen){
        mViewport.Width = maxScreenWidth;
        mViewport.Height = maxScreenHeight;
        mWindow = glfwCreateWindow(mViewport.Width, mViewport.Height, "BEE", mMonitor, nullptr);
    }
    else{
        mWindow = glfwCreateWindow(mViewport.Width, mViewport.Height, "BEE", nullptr, nullptr);
    }

    if (!mWindow)
    {
        LOG(LogCore, Fatal, "GLFW window could not be created");
        glfwTerminate();
        assert(false);
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(mWindow);

}

void Engine::Device::InitializeDevice()
{
    //DEBUG LAYERS
#if defined(_DEBUG)
    Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
    if (FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)))) {
        LOG(LogCore, Fatal, "Failed to get debug interface");
        assert(false && "Failed to get debug interface");
    }
    debugInterface->EnableDebugLayer();
#endif

    mViewport.TopLeftX = 0;
    mViewport.TopLeftY = 0;
    mViewport.MinDepth = 0.0f;
    mViewport.MaxDepth = 1.0f;

    mScissorRect.left = 0;
    mScissorRect.top = 0;
    mScissorRect.right = mViewport.Width;
    mScissorRect.bottom = mViewport.Height;

    ComPtr<IDXGIFactory4> dxgiFactory;
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
    if (FAILED(hr)) {
        LOG(LogCore, Fatal, "Failed to create dxgi factory");
        assert(false && "Failed to create dxgi factory");
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
                assert(false && "Failed to create device");
            }

            D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
            hr = mDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5,
                &options5, sizeof(options5));

            if (FAILED(hr)) {
                LOG(LogCore, Fatal, "Failed to pick adaptor");
                assert(false && "Failed to check raytracing support");
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
        assert(false && "Failed to create command queue");
    }

    //CREATE COMMAND ALLOCATOR
    for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
    {
        hr = mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator[i]));
        if (FAILED(hr)) {
            LOG(LogCore, Fatal, "Failed to create command allocator");
            assert(false && "Failed to create command allocator");
        }
    }

    //CREATE COMMAND LIST
    hr = mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator[0].Get(), NULL, IID_PPV_ARGS(&mCommandList));
    if (FAILED(hr)) {
        LOG(LogCore, Fatal, "Failed to create command list");
        assert(false && "Failed to create command list");
    }
    mCommandList->SetName(L"Main command list");

    //CREATE FENCES
    for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
    {
        hr = mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence[i]));
        if (FAILED(hr)) {
            LOG(LogCore, Fatal, "Failed to create fence");
            assert(false && "Failed to create fence");
        }

        mFenceValue[i] = 0; // set the initial fence value to 0
    }

    //CREATE FENCE EVENT
    mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (mFenceEvent == nullptr) {
        LOG(LogCore, Fatal, "Failed to create fence event");
        assert(false && "Failed to create fence event");
    }

    //CREATE SWAPCHAIN
    DXGI_MODE_DESC backBufferDesc = {};
    backBufferDesc.Width = mViewport.Width;
    backBufferDesc.Height = mViewport.Height;
    backBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    DXGI_SAMPLE_DESC sampleDesc = {};
    sampleDesc.Count = 1;

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = FRAME_BUFFER_COUNT;
    swapChainDesc.BufferDesc = backBufferDesc;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.OutputWindow = reinterpret_cast<HWND>(mWindow);
    swapChainDesc.SampleDesc = sampleDesc;
    swapChainDesc.Windowed = true;

    IDXGISwapChain* tempSwapChain;
    hr = dxgiFactory->CreateSwapChain(mCommandQueue.Get(), &swapChainDesc, &tempSwapChain);
    if (FAILED(hr)) {
        LOG(LogCore, Fatal, "Failed to create swap chain");
        assert(false && "Failed to create swap chain");
    }
    mSwapChain = static_cast<IDXGISwapChain3*>(tempSwapChain);

    //CREATE IMGUI HEAP
    imguiHeap = std::make_unique<DXDescHeap>(mDevice, 4, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, L"IMGUI DESCRIPTOR HEAP", D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

    //CREATE DESCRIPTOR HEAPS
    mDescriptorHeaps[RT_HEAP] = std::make_unique<DXDescHeap>(mDevice, 10, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, L"MAIN RENDER TARGETS HEAP");
    mDescriptorHeaps[DEPTH_HEAP] = std::make_unique<DXDescHeap>(mDevice, 4, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, L"DEPTH DESCRIPTOR HEAP");
    mDescriptorHeaps[RESOURCE_HEAP] = std::make_unique<DXDescHeap>(mDevice, 5000, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, L"RESOURCE HEAP", D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
    mDescriptorHeaps[SAMPLER_HEAP] = std::make_unique<DXDescHeap>(mDevice, 200, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, L"SAMPLER HEAP", D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

    //CREATE RENDER TARGETS
    for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
    {
        mResources[i] = std::make_unique<DXResource>();
        ComPtr<ID3D12Resource> res;
        HRESULT hr = mSwapChain->GetBuffer(i, IID_PPV_ARGS(&res));
        if (FAILED(hr)) {
            LOG(LogCore, Fatal, "Failed to get swapchain buffer");
            assert(false && "Failed to get swapchain buffer");
        }
        mResources[i]->SetResource(res);
        mDevice->CreateRenderTargetView(mResources[i]->Get(), nullptr, mDescriptorHeaps[RT_HEAP]->GetCPUHandle(i));
        mResources[i]->GetResource()->SetName(L"RENDER TARGET");
    }

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
    auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(mDepthFormat, mViewport.Width, mViewport.Height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    mResources[DEPTH_STENCIL_RSC] = std::make_unique<DXResource>(mDevice, heapProperties, resourceDesc, &depthOptimizedClearValue, "Depth/Stencil Resource");
    mResources[DEPTH_STENCIL_RSC]->ChangeState(mCommandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    mDevice->CreateDepthStencilView(mResources[DEPTH_STENCIL_RSC]->Get(), &depthStencilDesc, mDescriptorHeaps[DEPTH_HEAP]->GetCPUHandle(0));
}

void Engine::Device::WaitForFence(ComPtr<ID3D12Fence> fence, UINT64& fenceValue, HANDLE& fenceEvent)
{
    if (fence->GetCompletedValue() < fenceValue)
    {
        if (FAILED(fence->SetEventOnCompletion(fenceValue, fenceEvent))) {
            LOG(LogCore, Fatal, "Failed to set fence event on completion.");
            assert(false && "Failed to set fence event on completion.");
        }
        WaitForSingleObject(fenceEvent, INFINITE);
    }
    fenceValue++;
}

void Engine::Device::NewFrame() {

    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

    WaitForFence(mFence[mFrameIndex], mFenceValue[mFrameIndex], mFenceEvent);
    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

    if (FAILED(mCommandAllocator[mFrameIndex]->Reset())) {
        LOG(LogCore, Fatal, "Failed to reset command allocator");
        assert(false && "Failed to reset command allocator");
    }

    if (FAILED(mCommandList->Reset(mCommandAllocator[mFrameIndex].Get(), nullptr))) {
        LOG(LogCore, Fatal, "Failed to reset command list");
        assert(false && "Failed to reset command list");
    }

}

void Engine::Device::EndFrame()
{
    //CLOSE COMMAND LIST
    mCommandList->Close();
    ID3D12CommandList* ppCommandLists[] = { mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    mFenceValue[mFrameIndex]++;
    HRESULT hr = mCommandQueue->Signal(mFence[mFrameIndex].Get(), mFenceValue[mFrameIndex]);

    if (FAILED(hr)) {
        LOG(LogCore, Fatal, "Failed to signal fence");
        assert(false && "Failed to signal fence");
    }

    ImGui::EndFrame();
}


void Engine::Device::InitializeImGui()
{
    LOG(LogCore, Message, "Creating imgui context");

    glfwShowWindow(mWindow);
    mIsWindowOpen = true;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;
    ImGui::GetIO().ConfigViewportsNoDecoration = false;

    ImGui_ImplDX12_Init(mDevice.Get(), FRAME_BUFFER_COUNT,
        DXGI_FORMAT_R8G8B8A8_UNORM, imguiHeap->Get(),
        imguiHeap->GetCPUHandle(0),
        imguiHeap->GetGPUHandle(0));
    ImGui_ImplGlfw_InitForOther(mWindow, true);

    ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());

    std::filesystem::create_directories("Intermediate/Layout");
    ImGui::GetIO().IniFilename = "Intermediate/Layout/imgui.ini";
}

void Engine::Device::AllocateTexture(DXResource* rsc, D3D12_SHADER_RESOURCE_VIEW_DESC desc)
{
    mDevice->CreateShaderResourceView(rsc->Get(), &desc, mDescriptorHeaps[RESOURCE_HEAP]->GetCPUHandle(resourceCount));
    resourceCount++;
}
