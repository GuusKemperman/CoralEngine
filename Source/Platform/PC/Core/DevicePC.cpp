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

#include <stb_image/stb_image.h>

#include "Core/AssetManager.h"
#include "Platform/PC/Rendering/TexturePC.h"
#include "Platform/PC/Rendering/DX12Classes/DXDescHeap.h"
#include "Platform/PC/Rendering/DX12Classes/DXHeapHandle.h"
#include "Platform/PC/Rendering/DX12Classes/DXPipeline.h"
#include "Platform/PC/Rendering/DX12Classes/DXSignature.h"
#include "Utilities/StringFunctions.h"
#include "Rendering/FrameBuffer.h"

struct CE::Device::DXImpl
{
	enum DXResources {
		RT,
		DEPTH_STENCIL_RSC = FRAME_BUFFER_COUNT,
		NUM_RESOURCES = FRAME_BUFFER_COUNT + 1
	};
	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	ComPtr<ID3D12CommandQueue> mCommandQueue;
	ComPtr<ID3D12CommandQueue> mUploadCommandQueue;
	ComPtr<ID3D12CommandAllocator> mCommandAllocator[FRAME_BUFFER_COUNT];
	ComPtr<ID3D12CommandAllocator> mUploadCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList4> mCommandList;
	ComPtr<ID3D12GraphicsCommandList4> mUploadCommandList;

	ComPtr<ID3D12Fence> mFence[FRAME_BUFFER_COUNT];
	ComPtr<ID3D12Fence> mUploadFence;

	std::shared_ptr<DXDescHeap> mDescriptorHeaps[NUM_DESC_HEAPS];
	std::unique_ptr<DXResource> mResources[NUM_RESOURCES];
	const DXGI_FORMAT mDepthFormat = DXGI_FORMAT_D32_FLOAT;

	HANDLE mFenceEvent;
	HANDLE mUploadFenceEvent;
	UINT64 mFenceValue[FRAME_BUFFER_COUNT];
	UINT64 mUploadFenceValue;
	int mHeapResourceCount = 4;
	int frameBufferCount = FRAME_BUFFER_COUNT;
	int depthStencilCount = 1;
	std::vector<ComPtr<ID3D12Resource>> mResourcesToDeallocate;

	ComPtr<IDXGISwapChain3> mSwapChain;
	ComPtr<ID3D12Device5> mDevice;
	ComPtr<ID3D12RootSignature> mSignature;
	ComPtr<ID3D12RootSignature> mComputeSignature;
	ComPtr<ID3D12PipelineState> mGenMipmapsPipeline;

	DXHeapHandle mRenderTargetHandles[FRAME_BUFFER_COUNT];
	DXHeapHandle mDepthHandle;
	unsigned int mFrameIndex = 0;
};

static void WaitForFence(ComPtr<ID3D12Fence> fence, UINT64& fenceValue, HANDLE& fenceEvent)
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

CE::Device::Device() :
	mImpl(new DXImpl)
{
	InitializeWindow();
	InitializeDevice();
}

static constexpr size_t sNumOfIcons = 3;
static constexpr std::array<std::string_view, sNumOfIcons> GetEmbeddedIcons();

void CE::Device::InitializeWindow()
{
	LOG(LogCore, Verbose, "Initializing GLFW");

	if (!glfwInit())
	{
		LOG(LogCore, Fatal, "GLFW could not be initialized");
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

	mMonitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(mMonitor);

	auto maxScreenWidth = mode->width;
	auto maxScreenHeight = mode->height;

	mImpl->mViewport.Width = 1920;
	mImpl->mViewport.Height = 1080;
	mPreviousWidth = 1920;
	mPreviousHeight = 1080;

	mFullscreen = false;
	std::string applicationName{};

#ifdef EDITOR
	applicationName = "Coral Engine - ";
#endif

	const std::filesystem::path thisExe = FileIO::Get().GetPath(FileIO::Directory::ThisExecutable, "");
	applicationName += thisExe.filename().replace_extension().string();
	if (applicationName.empty())
	{
		applicationName += "Unnamed application";
	}
	if (mFullscreen)
	{
		mImpl->mViewport.Width = static_cast<FLOAT>(maxScreenWidth);
		mImpl->mViewport.Height = static_cast<FLOAT>(maxScreenHeight);
		mWindow = glfwCreateWindow(static_cast<int>(mImpl->mViewport.Width), static_cast<int>(mImpl->mViewport.Height), applicationName.c_str(), mMonitor, nullptr);
	}
	else
	{
		glfwWindowHint(GLFW_RESIZABLE, 1);
		mWindow = glfwCreateWindow(static_cast<int>(mImpl->mViewport.Width), static_cast<int>(mImpl->mViewport.Height), applicationName.c_str(), nullptr, nullptr);
	}

	if (mWindow == nullptr)
	{
		LOG(LogCore, Fatal, "GLFW window could not be created");
	}

	const std::array<std::string_view, sNumOfIcons> iconsHex = GetEmbeddedIcons();
	std::array<GLFWimage, sNumOfIcons> glfwImages{};

	for (size_t i = 0; i < sNumOfIcons; i++)
	{
		GLFWimage& image = glfwImages[i];
		const std::string data = StringFunctions::HexToBinary(iconsHex[i]);
		int channels;
		image.pixels = stbi_load_from_memory(reinterpret_cast<const uint8*>(data.data()), static_cast<int>(data.size()), &image.width, &image.height, &channels, 4);

		if (image.pixels == nullptr)
		{
			LOG(LogCore, Error, "Failed to load embedded icon {} for the application", i);
		}
	}

	glfwSetWindowIcon(mWindow, sNumOfIcons, glfwImages.data());

	for (size_t i = 0; i < sNumOfIcons; i++)
	{
		stbi_image_free(glfwImages[i].pixels);
	}

	glfwMakeContextCurrent(mWindow);
	glfwShowWindow(mWindow);
	mIsWindowOpen = true;
}

void CE::Device::InitializeDevice()
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

	mImpl->mViewport.TopLeftX = 0;
	mImpl->mViewport.TopLeftY = 0;
	mImpl->mViewport.MinDepth = 0.0f;
	mImpl->mViewport.MaxDepth = 1.0f;

	mImpl->mScissorRect.left = 0;
	mImpl->mScissorRect.top = 0;
	mImpl->mScissorRect.right = static_cast<LONG>(mImpl->mViewport.Width);
	mImpl->mScissorRect.bottom = static_cast<LONG>(mImpl->mViewport.Height);

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

			hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mImpl->mDevice));
			if (FAILED(hr)) {
				LOG(LogCore, Fatal, "Failed to create render device");
			}

			D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
			hr = mImpl->mDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5,
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
	hr = mImpl->mDevice->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&mImpl->mCommandQueue));
	if (FAILED(hr)) {
		LOG(LogCore, Fatal, "Failed to create command queue");
	}

	hr = mImpl->mDevice->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&mImpl->mUploadCommandQueue));
	if (FAILED(hr))
	{
		LOG(LogCore, Fatal, "Failed to create upload command queue");
	}

	//CREATE COMMAND ALLOCATOR
	for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		hr = mImpl->mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mImpl->mCommandAllocator[i]));
		if (FAILED(hr))
		{
			LOG(LogCore, Fatal, "Failed to create command allocator");
		}
	}

	hr = mImpl->mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mImpl->mUploadCommandAllocator));
	if (FAILED(hr))
	{
		LOG(LogCore, Fatal, "Failed to create upload command allocator");
	}

	//CREATE COMMAND LIST
	hr = mImpl->mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mImpl->mCommandAllocator[0].Get(), NULL, IID_PPV_ARGS(&mImpl->mCommandList));
	if (FAILED(hr))
	{
		LOG(LogCore, Fatal, "Failed to create command list");
	}
	mImpl->mCommandList->SetName(L"Main command list");

	hr = mImpl->mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mImpl->mUploadCommandAllocator.Get(), NULL, IID_PPV_ARGS(&mImpl->mUploadCommandList));
	if (FAILED(hr))
	{
		LOG(LogCore, Fatal, "Failed to create upload command list");
	}
	mImpl->mUploadCommandList->SetName(L"Upload command list");
	mImpl->mUploadCommandList->Close();

	//CREATE FENCES
	for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		hr = mImpl->mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mImpl->mFence[i]));
		if (FAILED(hr))
		{
			LOG(LogCore, Fatal, "Failed to create fence");
		}

		mImpl->mFenceValue[i] = 0; // set the initial fence value to 0
	}

	hr = mImpl->mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mImpl->mUploadFence));
	if (FAILED(hr))
	{
		LOG(LogCore, Fatal, "Failed to upload create fence");
	}

	mImpl->mUploadFenceValue = 0; // set the initial fence value to 0


	//CREATE FENCE EVENT
	mImpl->mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (mImpl->mFenceEvent == nullptr)
	{
		LOG(LogCore, Fatal, "Failed to create fence event");
	}

	mImpl->mUploadFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (mImpl->mUploadFenceEvent == nullptr)
	{
		LOG(LogCore, Fatal, "Failed to create upload fence event");
	}

	//CREATE SWAPCHAIN
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = static_cast<UINT>(mImpl->mViewport.Width);
	swapChainDesc.Height =static_cast<UINT>(mImpl->mViewport.Height);
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc = {1, 0};
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = FRAME_BUFFER_COUNT;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

	ComPtr<IDXGISwapChain1> tempSwapChain;
	hr = dxgiFactory->CreateSwapChainForHwnd(
		mImpl->mCommandQueue.Get(), reinterpret_cast<HWND>(glfwGetWin32Window(mWindow)), &swapChainDesc, nullptr, nullptr,
		&tempSwapChain);

	if (FAILED(hr))
	{
		LOG(LogCore, Fatal, "Failed to create swap chain");
	}
	tempSwapChain.As(&mImpl->mSwapChain);

	//CREATE DESCRIPTOR HEAPS
	mImpl->mDescriptorHeaps[RT_HEAP] = DXDescHeap::Construct(mImpl->mDevice, 2000, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, L"MAIN RENDER TARGETS HEAP");
	mImpl->mDescriptorHeaps[DEPTH_HEAP] = DXDescHeap::Construct(mImpl->mDevice, 2000, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, L"DEPTH DESCRIPTOR HEAP");
	mImpl->mDescriptorHeaps[RESOURCE_HEAP] = DXDescHeap::Construct(mImpl->mDevice, 50000, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, L"RESOURCE HEAP", D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
	mImpl->mDescriptorHeaps[SAMPLER_HEAP] = DXDescHeap::Construct(mImpl->mDevice, 200, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, L"SAMPLER HEAP", D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

	//CREATE RENDER TARGETS
	for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		mImpl->mResources[i] = std::make_unique<DXResource>();
		ComPtr<ID3D12Resource> res;
		hr = mImpl->mSwapChain->GetBuffer(i, IID_PPV_ARGS(&res));
		if (FAILED(hr))
		{
			LOG(LogCore, Fatal, "Failed to get swapchain buffer");
		}
		mImpl->mResources[i]->SetResource(res);
		mImpl->mRenderTargetHandles[i] = mImpl->mDescriptorHeaps[RT_HEAP]->AllocateRenderTarget(mImpl->mResources[i].get(), mImpl->mDevice.Get(), nullptr);
		mImpl->mResources[i]->GetResource()->SetName(L"RENDER TARGET");
	}

	//CREATE ROOT SIGNATURE
	mImpl->mSignature = DXSignatureBuilder(10)
		.AddCBuffer(0, D3D12_SHADER_VISIBILITY_ALL)   //0  //Camera matrices
		.AddCBuffer(1, D3D12_SHADER_VISIBILITY_VERTEX)//1  //Model matrices
		.AddCBuffer(2, D3D12_SHADER_VISIBILITY_VERTEX)//2  //Bone matrices
		.AddCBuffer(3, D3D12_SHADER_VISIBILITY_ALL)   //3  //Light info 
		.AddCBuffer(4, D3D12_SHADER_VISIBILITY_PIXEL) //4  //Material info
		.AddCBuffer(5, D3D12_SHADER_VISIBILITY_PIXEL) //5  //Color multiplier
		.AddCBuffer(6, D3D12_SHADER_VISIBILITY_PIXEL) //6  //Camera clustering buffer
		.AddCBuffer(7, D3D12_SHADER_VISIBILITY_PIXEL) //7  // Cluster info buffer

		.AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0)//8   //Base color tex
		.AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1)//9   //Emissive tex
		.AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2)//10  //NormalTex
		.AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3)//11  //Occlusion texture
		.AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4)//12  
		.AddTable(D3D12_SHADER_VISIBILITY_ALL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5)//13  //Directonal lights buffer
		.AddTable(D3D12_SHADER_VISIBILITY_ALL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 6)//14  //Point lights buffer
		.AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 7)//15  //Light grid buffer
		.AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 8)//16  //Light indices buffer
		.AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 9)//17  //Shadow maps

		.AddCBuffer(8, D3D12_SHADER_VISIBILITY_PIXEL) //18  //Fog info buffer
		.AddCBuffer(9, D3D12_SHADER_VISIBILITY_PIXEL) //19  //Particle info buffer
		.Add32BitConstant(10, D3D12_SHADER_VISIBILITY_PIXEL, 5) //20 //Root constant for general uses
		.AddSampler(0, D3D12_SHADER_VISIBILITY_PIXEL, D3D12_TEXTURE_ADDRESS_MODE_WRAP) //21  //Sampler
		.AddSampler(1, D3D12_SHADER_VISIBILITY_PIXEL,
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
			D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
			D3D12_COMPARISON_FUNC_LESS_EQUAL) //22  //Sampler
		.Build(mImpl->mDevice, L"MAIN ROOT SIGNATURE");

	//COMPUTE ROOT SIGNATURE
	//CREATE COMPUTE ROOT SIGNATURE
	mImpl->mComputeSignature = DXSignatureBuilder(8)
		.AddCBuffer(0, D3D12_SHADER_VISIBILITY_ALL) // Cluster info 0
		.AddCBuffer(1, D3D12_SHADER_VISIBILITY_ALL) // Camera info 1
		.AddCBuffer(2, D3D12_SHADER_VISIBILITY_ALL) // Cluster camera info 2
		.AddCBuffer(3, D3D12_SHADER_VISIBILITY_ALL) // Light info 3
		.AddCBuffer(4, D3D12_SHADER_VISIBILITY_ALL) // Model matrices 4
		.AddCBuffer(5, D3D12_SHADER_VISIBILITY_ALL) // Pixel color for cluster culling 5

		.AddTable(D3D12_SHADER_VISIBILITY_ALL, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0) //6
		.AddTable(D3D12_SHADER_VISIBILITY_ALL, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1) //7
		.AddTable(D3D12_SHADER_VISIBILITY_ALL, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 2) //8
		.AddTable(D3D12_SHADER_VISIBILITY_ALL, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 3) //9

		.AddTable(D3D12_SHADER_VISIBILITY_ALL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0) //10
		.AddTable(D3D12_SHADER_VISIBILITY_ALL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1) //11
		.AddTable(D3D12_SHADER_VISIBILITY_ALL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2) //12
		.AddTable(D3D12_SHADER_VISIBILITY_ALL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3) //13
		.AddSampler(0, D3D12_SHADER_VISIBILITY_ALL, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_FILTER_MIN_MAG_MIP_LINEAR)//14
		.Build(mImpl->mDevice, L"COMPUTE ROOT SIGNATURE");

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
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(mImpl->mDepthFormat, static_cast<UINT>(mImpl->mViewport.Width), static_cast<UINT>(mImpl->mViewport.Height), 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	mImpl->mResources[DXImpl::DEPTH_STENCIL_RSC] = std::make_unique<DXResource>(mImpl->mDevice, heapProperties, resourceDesc, &depthOptimizedClearValue, "Depth/Stencil Resource");
	mImpl->mResources[DXImpl::DEPTH_STENCIL_RSC]->ChangeState(mImpl->mCommandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	mImpl->mDepthHandle = mImpl->mDescriptorHeaps[DEPTH_HEAP]->AllocateDepthStencil(mImpl->mResources[DXImpl::DEPTH_STENCIL_RSC].get(), mImpl->mDevice.Get(), &depthStencilDesc);

	FileIO& fileIO = FileIO::Get();
	std::string shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/MipmapGen.hlsl");
	ComPtr<ID3DBlob> cs = DXPipelineBuilder::ShaderToBlob(shaderPath.c_str(), "cs_5_0");
	mImpl->mGenMipmapsPipeline = DXPipelineBuilder()
		.SetComputeShader(cs->GetBufferPointer(), cs->GetBufferSize())
		.Build(mImpl->mDevice, mImpl->mComputeSignature, L"GENERATE MIPMAPS COMPUTE SHADER");


	SubmitCommands();
}

void CE::Device::UpdateRenderTarget()
{
	if (mImpl->mViewport.Width <= 0 || mImpl->mViewport.Height <= 0)
		return;

	mImpl->mResources[0] = nullptr;
	mImpl->mResources[1] = nullptr;
	mImpl->mResources[DXImpl::DEPTH_STENCIL_RSC] = nullptr;

	HRESULT hr = mImpl->mSwapChain->ResizeBuffers(FRAME_BUFFER_COUNT, static_cast<UINT>(mImpl->mViewport.Width), static_cast<UINT>(mImpl->mViewport.Height), DXGI_FORMAT_R8G8B8A8_UNORM, 0);
	if (FAILED(hr))
	{
		LOG(LogCore, Fatal, "Failed to resize framebuffer");
	}

	for (int i = 0; i < FRAME_BUFFER_COUNT; i++) {
		mImpl->mResources[i] = std::make_unique<DXResource>();
		ComPtr<ID3D12Resource> res;
		hr = mImpl->mSwapChain->GetBuffer(i, IID_PPV_ARGS(&res));
		if (FAILED(hr))
		{
			LOG(LogCore, Fatal, "Failed to get swapchain buffer");
		}
		mImpl->mResources[i]->SetResource(res);
		mImpl->mRenderTargetHandles[i] = mImpl->mDescriptorHeaps[RT_HEAP]->AllocateRenderTarget(mImpl->mResources[i].get(), mImpl->mDevice.Get(), nullptr);
		mImpl->mResources[i]->GetResource()->SetName(L"RENDER TARGET");
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
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(mImpl->mDepthFormat, static_cast<UINT>(mImpl->mViewport.Width), static_cast<UINT>(mImpl->mViewport.Height), 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	mImpl->mResources[DXImpl::DEPTH_STENCIL_RSC] = std::make_unique<DXResource>(mImpl->mDevice, heapProperties, resourceDesc, &depthOptimizedClearValue, "Depth/Stencil Resource");
	mImpl->mResources[DXImpl::DEPTH_STENCIL_RSC]->ChangeState(mImpl->mCommandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	mImpl->mDepthHandle = mImpl->mDescriptorHeaps[DEPTH_HEAP]->AllocateDepthStencil(mImpl->mResources[DXImpl::DEPTH_STENCIL_RSC].get(), mImpl->mDevice.Get(), &depthStencilDesc);

	mImpl->mFrameIndex = mImpl->mSwapChain->GetCurrentBackBufferIndex();
}

void CE::Device::NewFrame() {

	int width, height;
	mIsWindowOpen = !glfwWindowShouldClose(mWindow);

	glfwGetWindowSize(mWindow, &width, &height);
	if (width != mPreviousWidth || height != mPreviousHeight) {
		mImpl->mViewport.Width = static_cast<float>(width);
		mImpl->mViewport.Height = static_cast<float>(height);
		mPreviousWidth = width;
		mPreviousHeight = height;
		mImpl->mScissorRect.right = static_cast<LONG>(mImpl->mViewport.Width);
		mImpl->mScissorRect.bottom = static_cast<LONG>(mImpl->mViewport.Height);
		mUpdateWindow = true;
	}

	glfwGetWindowPos(mWindow, &mPreviousPosX, &mPreviousPosY);

	Device& engineDevice = Device::Get();
	if (engineDevice.GetDisplaySize().x <= 0 || engineDevice.GetDisplaySize().y <= 0)
		return;

#ifdef EDITOR
	ImGui::GetIO().DisplaySize.x = mImpl->mViewport.Width;
	ImGui::GetIO().DisplaySize.y = mImpl->mViewport.Height;
#endif // EDITOR

	StartRecordingCommands();

#ifdef EDITOR
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();
#endif // EDITOR

	SendTexturesToGPU();
	BindSwapchainRT();
	glm::vec4 clearColor(0.329f, 0.329f, 0.329f, 1.f);
	mImpl->mDescriptorHeaps[RT_HEAP]->ClearRenderTarget(mImpl->mCommandList, mImpl->mRenderTargetHandles[mImpl->mFrameIndex], &clearColor[0]);
	mImpl->mDescriptorHeaps[DEPTH_HEAP]->ClearDepthStencil(mImpl->mCommandList, mImpl->mDepthHandle);
}

void CE::Device::EndFrame()
{
	mImpl->mDescriptorHeaps[RT_HEAP]->BindRenderTargets(mImpl->mCommandList, &mImpl->mRenderTargetHandles[mImpl->mFrameIndex], mImpl->mDepthHandle);

	auto* desc_ptr = mImpl->mDescriptorHeaps[RESOURCE_HEAP]->Get();
	mImpl->mCommandList->SetDescriptorHeaps(1, &desc_ptr);

#ifdef EDITOR
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), mImpl->mCommandList.Get());

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
	mImpl->mResources[mImpl->mFrameIndex]->ChangeState(mImpl->mCommandList, D3D12_RESOURCE_STATE_PRESENT);

	SubmitCommands();

	if (FAILED(mImpl->mSwapChain->Present(0, 0)))
	{
		LOG(LogCore, Fatal, "Failed to present");
	}

#ifdef EDITOR
	ImGui::EndFrame();
#endif // EDITOR

	WaitForFence(mImpl->mFence[mImpl->mFrameIndex], mImpl->mFenceValue[mImpl->mFrameIndex], mImpl->mFenceEvent);

	if (mUpdateWindow)
	{
		mImpl->mResources[0] = nullptr;
		mImpl->mResources[1] = nullptr;
		mImpl->mResources[DXImpl::DEPTH_STENCIL_RSC] = nullptr;
	}

	mImpl->mResourcesToDeallocate.clear();

	if (mUpdateWindow)
	{
		int otherFrameIndex = mImpl->mFrameIndex == 0 ? 1 : 0;
		mImpl->mFenceValue[otherFrameIndex]++;
		mImpl->mCommandQueue->Signal(mImpl->mFence[otherFrameIndex].Get(), mImpl->mFenceValue[otherFrameIndex]);
		WaitForFence(mImpl->mFence[otherFrameIndex], mImpl->mFenceValue[otherFrameIndex], mImpl->mFenceEvent);
		StartRecordingCommands();
		UpdateRenderTarget();
		SubmitCommands();
		WaitForFence(mImpl->mFence[mImpl->mFrameIndex], mImpl->mFenceValue[mImpl->mFrameIndex], mImpl->mFenceEvent);
		mUpdateWindow = false;
	}
}

void* CE::Device::GetWindow()
{
	return mWindow;
}

void* CE::Device::GetSwapchain()
{
	return mImpl->mSwapChain.Get();
}

void* CE::Device::GetDevice()
{
	return mImpl->mDevice.Get();
}

void* CE::Device::GetCommandList()
{
	return mImpl->mCommandList.Get();
}

void* CE::Device::GetUploadCommandList()
{
	return mImpl->mUploadCommandList.Get();
}

void* CE::Device::GetCommandQueue()
{
	return mImpl->mCommandQueue.Get();
}

void* CE::Device::GetSignature()
{
	return mImpl->mSignature.Get();
}

void* CE::Device::GetComputeSignature()
{
	return mImpl->mComputeSignature.Get();
}

void* CE::Device::GetMipmapPipeline()
{
	return mImpl->mGenMipmapsPipeline.Get();
}

void CE::Device::SubmitCommands()
{
	//CLOSE COMMAND LIST
	mImpl->mCommandList->Close();
	ID3D12CommandList* ppCommandLists[] = { mImpl->mCommandList.Get() };
	mImpl->mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	mImpl->mFenceValue[mImpl->mFrameIndex]++;
	HRESULT hr = mImpl->mCommandQueue->Signal(mImpl->mFence[mImpl->mFrameIndex].Get(), mImpl->mFenceValue[mImpl->mFrameIndex]);

	if (FAILED(hr))
	{
		LOG(LogCore, Fatal, "Failed to signal fence");
	}

}

void CE::Device::StartRecordingCommands()
{
	mImpl->mFrameIndex = mImpl->mSwapChain->GetCurrentBackBufferIndex();

	if (FAILED(mImpl->mCommandAllocator[mImpl->mFrameIndex]->Reset()))
	{
		LOG(LogCore, Fatal, "Failed to reset command allocator");
	}

	if (FAILED(mImpl->mCommandList->Reset(mImpl->mCommandAllocator[mImpl->mFrameIndex].Get(), nullptr)))
	{
		LOG(LogCore, Fatal, "Failed to reset command list");
	}
}

void CE::Device::SendTexturesToGPU()
{
	AssetHandle defaultTexture = Texture::TryGetDefaultTexture();

	if (defaultTexture != nullptr
		&& !defaultTexture->WasSendToGPU())
	{
		defaultTexture->SendToGPU();
	}

	for (WeakAssetHandle<Texture> weakHandle : AssetManager::Get().GetAllAssets<Texture>())
	{
		if (!weakHandle.IsLoaded())
		{
			continue;
		}

		AssetHandle<Texture> texture{ weakHandle };

		if (texture->IsReadyToBeSentToGpu())
		{
			texture->SendToGPU();
		}
	}
}

void CE::Device::DXImplDeleter::operator()(DXImpl* impl) const
{
	delete impl;
}

bool CE::Device::StartUploadCommands()
{
	if (mUploadCommandListOpen)
		return false;

	if (FAILED(mImpl->mUploadCommandAllocator->Reset()))
	{
		LOG(LogCore, Fatal, "Failed to reset upload command allocator");
	}

	if (FAILED(mImpl->mUploadCommandList->Reset(mImpl->mUploadCommandAllocator.Get(), nullptr)))
	{
		LOG(LogCore, Fatal, "Failed to reset upload command list");
	}

	mUploadCommandListOpen = true;
	return true;
}

void CE::Device::SubmitUploadCommands()
{
	//CLOSE COMMAND LIST
	mImpl->mUploadCommandList->Close();
	ID3D12CommandList* ppCommandLists[] = { mImpl->mUploadCommandList.Get() };
	mImpl->mUploadCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	mImpl->mUploadFenceValue++;
	HRESULT hr = mImpl->mUploadCommandQueue->Signal(mImpl->mUploadFence.Get(), mImpl->mUploadFenceValue);

	WaitForFence(mImpl->mUploadFence, mImpl->mUploadFenceValue, mImpl->mUploadFenceEvent);

	if (FAILED(hr))
	{
		LOG(LogCore, Fatal, "Failed to signal upload fence");
	}
	else
		mUploadCommandListOpen = false;
}

void CE::Device::AddToDeallocation(ComPtr<ID3D12Resource>&& res)
{
	mImpl->mResourcesToDeallocate.emplace_back(std::move(res));
}

void CE::Device::BindSwapchainRT()
{
	mImpl->mCommandList->RSSetViewports(1, &mImpl->mViewport);
	mImpl->mCommandList->RSSetScissorRects(1, &mImpl->mScissorRect);
	mImpl->mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	mImpl->mResources[mImpl->mFrameIndex]->ChangeState(mImpl->mCommandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	mImpl->mDescriptorHeaps[RT_HEAP]->BindRenderTargets(mImpl->mCommandList, &mImpl->mRenderTargetHandles[mImpl->mFrameIndex], mImpl->mDepthHandle);
}

void CE::Device::ResolveMsaa(FrameBuffer & msaaFramebuffer)
{
	if (msaaFramebuffer.GetSize() != glm::vec2(mImpl->mViewport.Width, mImpl->mViewport.Height))
		return;

	mImpl->mResources[mImpl->mFrameIndex]->ChangeState(mImpl->mCommandList, D3D12_RESOURCE_STATE_RESOLVE_DEST);

	msaaFramebuffer.PrepareMsaaForResolve();

	mImpl->mCommandList->ResolveSubresource(
		mImpl->mResources[mImpl->mFrameIndex]->Get(),
		0,
		msaaFramebuffer.GetResource().Get(),
		0,
		DXGI_FORMAT_R8G8B8A8_UNORM);
}

void CE::Device::CopyToRenderTargets(FrameBuffer & source)
{
	source.SetAsCopySource();
	Device& engineDevice = Device::Get();
	ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
	mImpl->mResources[engineDevice.GetFrameIndex()]->ChangeState(commandList, D3D12_RESOURCE_STATE_COPY_DEST);
	commandList->CopyResource(mImpl->mResources[engineDevice.GetFrameIndex()]->Get(), source.GetResource().Get());
}

glm::vec2 CE::Device::GetDisplaySize()
{
	return glm::vec2(mImpl->mViewport.Width, mImpl->mViewport.Height);
}

glm::vec2 CE::Device::GetWindowPosition()
{
	return glm::vec2{ static_cast<float>(mPreviousPosX), static_cast<float>(mPreviousPosY) };
}

std::shared_ptr<DXDescHeap> CE::Device::GetDescriptorHeap(int heap)
{
	return mImpl->mDescriptorHeaps[heap];
}

int CE::Device::GetFrameIndex()
{
	return mImpl->mFrameIndex;
}

#ifdef EDITOR
void CE::Device::CreateImguiContext()
{
	LOG(LogCore, Verbose, "Creating imgui context");

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::GetIO().ConfigFlags |=
		ImGuiConfigFlags_DockingEnable
		| ImGuiConfigFlags_ViewportsEnable
		| ImGuiConfigFlags_NavEnableGamepad
		| ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::GetIO().ConfigViewportsNoDecoration = false;
	ImGui::GetIO().DisplaySize.x = mImpl->mViewport.Width;
	ImGui::GetIO().DisplaySize.y = mImpl->mViewport.Height;

	ImGui_ImplDX12_Init(mImpl->mDevice.Get(), FRAME_BUFFER_COUNT,
		DXGI_FORMAT_R8G8B8A8_UNORM, mImpl->mDescriptorHeaps[RESOURCE_HEAP]->Get(),
		mImpl->mDescriptorHeaps[RESOURCE_HEAP]->Get()->GetCPUDescriptorHandleForHeapStart(),
		mImpl->mDescriptorHeaps[RESOURCE_HEAP]->Get()->GetGPUDescriptorHandleForHeapStart());
	ImGui_ImplGlfw_InitForOther(mWindow, true);
	ImGui_ImplGlfw_SetCallbacksChainForAllWindows(true);

	ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());

	std::filesystem::create_directories("Intermediate/Layout");
	ImGui::GetIO().IniFilename = "Intermediate/Layout/imgui.ini";
}
#endif

constexpr std::array<std::string_view, sNumOfIcons> GetEmbeddedIcons()
{
	return
	{
		std::string_view
		{
			// 48x48
			"89504E470D0A1A0A0000000D49484452"
			"000000300000003001030000006DCC6B"
			"C4000000017352474200AECE1CE90000"
			"000467414D410000B18F0BFC61050000"
			"0006504C54453EDED9FFFFFFF630CE2B"
			"000000097048597300000EC200000EC2"
			"0115284A80000000E84944415428CF75"
			"D0B18AC2401006E07F19D86D02692DA2"
			"D75D9DED52C8F92A81F49AE38A6B2C56"
			"52D8C8D56773BE8290C6721FC087D860"
			"61A7011B0FCED399A0A5D37CB00C3BF3"
			"0F9ED44BDB71FD1728C48E8DA1029300"
			"259329973125ADF9019ED61A50BEA80C"
			"F7BBA2220742515B07AD8B3AF51DF088"
			"F4A44E7244AF937AC87CEE4ED31CBD8F"
			"6697A7E8954D1584F7D98179B3DFC2C0"
			"6E04D8F398C96DE85AAC174676B54D11"
			"8D96AB057FA62ED51F83ABA061E66719"
			"4B5F3C96CCEF8FE29566970D6FA67CBF"
			"255EDEC7AD960C264492C80449949820"
			"F962D3485ADA1F25FBFD128FBB3C29E0"
			"0633F14FAA8F44058B0000000049454E"
			"44AE426082"
		},
		{
			// 32x32
			"89504E470D0A1A0A0000000D49484452"
			"0000002000000020010300000049B4E8"
			"B7000000017352474200AECE1CE90000"
			"000467414D410000B18F0BFC61050000"
			"0006504C54453EDED9FFFFFFF630CE2B"
			"000000097048597300000EC200000EC2"
			"0115284A80000000874944415418D363"
			"60606C60606060DE0023929BE1C46603"
			"063628C1C0C096BF5986412D7FB30D43"
			"F9E3CF350CC50F1F2730143C6C7EC050"
			"50D8F88EC1CE70C63B0639C31DEF1818"
			"0C7FE43124183E004A181E78C0606F38"
			"FF0C83FDCCF93D0C8C7F9BFF3030FC07"
			"11ECED3F18D898FB808C9FF3800EF8BB"
			"03689BEC0720C10F22D81FC089070C00"
			"150931AC364E68230000000049454E44"
			"AE426082"
		},
		{
			// 16x16
			"89504E470D0A1A0A0000000D49484452"
			"00000010000000100103000000253D6D"
			"22000000017352474200AECE1CE90000"
			"000467414D410000B18F0BFC61050000"
			"0006504C54453EDED9FFFFFFF630CE2B"
			"000000097048597300000EC200000EC2"
			"0115284A80000000374944415418D363"
			"00024E0106CE1006CD0006ED3006C929"
			"0CBE520CA95E0CAC33184AB318E49731"
			"38CB30D81630B01F60606E60606E0000"
			"A71F084E8E1D12830000000049454E44"
			"AE426082"
		}
	};
}
