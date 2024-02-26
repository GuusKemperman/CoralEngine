#pragma once
#include "Core/EngineSubsystem.h"
#include "Platform/PC/Rendering/DX12Classes/DXDefines.h"
#include "glm/glm.hpp"

#pragma warning(push)
#pragma warning(disable: 4005)
#include <wrl.h>
#define NOMINMAX
#include <Windows.h>
#pragma warning(pop) 
using namespace Microsoft::WRL;


struct GLFWwindow;
struct GLFWmonitor;
struct DXDescHeap;

namespace Engine
{
    class Device final : 
        public EngineSubsystem<Device>
    {
        friend EngineSubsystem;
        Device();
        ~Device() final override;

    public:
        void NewFrame();
        void EndFrame();
        bool ShouldClose() const { return !mIsWindowOpen; }

        void* GetWindow() { return mWindow; }
        void* GetSwapchain() { return mSwapChain.Get(); }
        void* GetDevice() { return mDevice.Get(); }
        void* GetCommandList() { return mCommandList.Get(); }
        void* GetCommandQueue() { return mCommandQueue.Get(); }

        int GetWidth() { return mViewport.Width; }
        int GetHeight() { return mViewport.Height; }

    //Platform specific heap
    public:
        std::shared_ptr<DXDescHeap> GetDescriptorHeap(int heap) { return mDescriptorHeaps[heap]; }
        void AllocateTexture(DXResource* rsc, D3D12_SHADER_RESOURCE_VIEW_DESC desc);

    private:
        void InitializeWindow();
        void InitializeDevice();
        void InitializeImGui();
        void WaitForFence(ComPtr<ID3D12Fence> fence, UINT64& fenceValue, HANDLE& fenceEvent);

    private:
        bool mIsWindowOpen{};
        GLFWwindow* mWindow;
        GLFWmonitor* mMonitor;
        D3D12_VIEWPORT mViewport;
        D3D12_RECT mScissorRect;
        unsigned int mFrameIndex = 0;
        bool mFullscreen = false;

        ComPtr<ID3D12CommandQueue> mCommandQueue;
        ComPtr<ID3D12CommandAllocator> mCommandAllocator[FRAME_BUFFER_COUNT];
        ComPtr<ID3D12GraphicsCommandList4> mCommandList;
        ComPtr<ID3D12Fence> mFence[FRAME_BUFFER_COUNT];
        std::shared_ptr<DXDescHeap> mDescriptorHeaps[NUM_DESC_HEAPS];
        std::unique_ptr<DXResource> mResources[NUM_RESOURCES];
        const DXGI_FORMAT mDepthFormat = DXGI_FORMAT_D32_FLOAT;

        HANDLE mFenceEvent;
        UINT64 mFenceValue[FRAME_BUFFER_COUNT];
        int resourceCount = NUM_RESOURCES;

        ComPtr<IDXGISwapChain3> mSwapChain;
        ComPtr<ID3D12Device5> mDevice;
        std::unique_ptr<DXDescHeap> imguiHeap;
    };
}


