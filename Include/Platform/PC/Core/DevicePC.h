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
        bool ShouldClose() const { return !mIsWindowOpen; }

        void* GetWindow() { return mWindow; }
        void* GetSwapchain() { return mSwapChain.Get(); }
        void* GetDevice() { return mDevice.Get(); }
        void* GetCommandList() { return mCommandList.Get(); }
        void* GetUploadCommandList() { return mUploadCommandList.Get(); }
        void* GetCommandQueue() { return mCommandQueue.Get(); }
        void CreateImguiContext();
        void StartUploadCommands();
        void SubmitUploadCommands();

        glm::vec2 GetDisplaySize() { return glm::vec2(mViewport.Width, mViewport.Height); }
    //Platform specific heap
    public:
        std::shared_ptr<DXDescHeap> GetDescriptorHeap(int heap) { return mDescriptorHeaps[heap]; }
        int AllocateTexture(DXResource* rsc, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc);
        void AllocateTexture(DXResource* rsc, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc, unsigned int slot);
        int AllocateFramebuffer(DXResource* rsc, const D3D12_RENDER_TARGET_VIEW_DESC& desc);
        int AllocateDepthStencil(DXResource* rsc, const D3D12_DEPTH_STENCIL_VIEW_DESC& desc);
        void AllocateDepthStencil(DXResource* rsc, const D3D12_DEPTH_STENCIL_VIEW_DESC& desc, unsigned int slot);
        void AllocateFramebuffer(DXResource* rsc, const D3D12_RENDER_TARGET_VIEW_DESC& desc, unsigned int slot);
        int GetFrameIndex() { return mFrameIndex; }

    private:
        void InitializeWindow();
        void InitializeDevice();
        void WaitForFence(ComPtr<ID3D12Fence> fence, UINT64& fenceValue, HANDLE& fenceEvent);
        void UpdateRenderTarget();
        void SubmitCommands();
        void StartRecordingCommands();


    private:
        bool mIsWindowOpen{};
        GLFWwindow* mWindow;
        GLFWmonitor* mMonitor;
        D3D12_VIEWPORT mViewport;
        D3D12_RECT mScissorRect;
        unsigned int mFrameIndex = 0;
        bool mFullscreen = false;

        int mPreviousWidth, mPreviousHeight;

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
        int mHeapResourceCount = TEX_START;
        int frameBufferCount = RT_COUNT;
        int depthStencilCount = 1;

        ComPtr<IDXGISwapChain3> mSwapChain;
        ComPtr<ID3D12Device5> mDevice;

        bool mUpdateWindow = false;
    };
}


