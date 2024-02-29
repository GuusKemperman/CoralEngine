#pragma once
#include "Core/EngineSubsystem.h"
#include "Platform/PC/Rendering/DX12Classes/DXDefines.h"
#include "glm/glm.hpp"
#include "Platform/PC/Rendering/DX12Classes/DXResource.h"

struct GLFWwindow;
struct GLFWmonitor;
class DXDescHeap;

namespace Engine
{
    class Device final : 
        public EngineSubsystem<Device>
    {
        friend EngineSubsystem;
        Device();
        ~Device() final override {};

    public:
        void NewFrame();
        void EndFrame();
        void SubmitCommands();
        void StartRecordingCommands();
        bool ShouldClose() const { return !mIsWindowOpen; }

        void* GetWindow() { return mWindow; }
        void* GetSwapchain() { return mSwapChain.Get(); }
        void* GetDevice() { return mDevice.Get(); }
        void* GetCommandList() { return mCommandList.Get(); }
        void* GetCommandQueue() { return mCommandQueue.Get(); }
        void CreateImguiContext();

        //int GetWidth() const{ return static_cast<int>(mViewport.Width); }
        //int GetHeight() const { return static_cast<int>(mViewport.Height); }
        glm::vec2 GetDisplaySize() { return glm::vec2(mViewport.Width, mViewport.Height); }
    //Platform specific heap
    public:
        std::shared_ptr<DXDescHeap> GetDescriptorHeap(int heap) { return mDescriptorHeaps[heap]; }
        int AllocateTexture(DXResource* rsc, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc);
        int AllocateFramebuffer(DXResource* rsc, const D3D12_RENDER_TARGET_VIEW_DESC& desc);
        void AllocateFramebuffer(DXResource* rsc, const D3D12_RENDER_TARGET_VIEW_DESC& desc, unsigned int slot);
        int GetFrameIndex() { return mFrameIndex; }

    private:
        void InitializeWindow();
        void InitializeDevice();
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
        int frameBufferCount = RT_COUNT;

        ComPtr<IDXGISwapChain3> mSwapChain;
        ComPtr<ID3D12Device5> mDevice;
    };
}


