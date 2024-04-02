#pragma once
#include "Core/EngineSubsystem.h"
#include "Platform/PC/Rendering/DX12Classes/DXDefines.h"
#include "glm/glm.hpp"
#include "Platform/PC/Rendering/DX12Classes/DXResource.h"
#include "Platform/PC/Rendering/DX12Classes/DXSignature.h"
#include "Platform/PC/Rendering/DX12Classes/DXHeapHandle.h"
#include "Platform/PC/Rendering/DX12Classes/DXConstBuffer.h"
#include "Platform/PC/Rendering/DX12Classes/DXPipeline.h"

struct GLFWwindow;
struct GLFWmonitor;
class DXDescHeap;

namespace CE
{
    class Engine;

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
        void* GetSignature() { return mSignature.get(); }
        void* GetComputeSignature() { return mComputeSignature.get(); }
        void* GetMipmapPipeline() { return mGenMipmapsPipeline.get(); }

#ifdef EDITOR
        void CreateImguiContext();
#endif // EDITOR

		void StartUploadCommands();
        void SubmitUploadCommands();
        void AddToDeallocation(ComPtr<ID3D12Resource>&& res);

        glm::vec2 GetDisplaySize() { return glm::vec2(mViewport.Width, mViewport.Height); }

        /**
         * \brief Some build/testing servers do not support graphics. This function can be used to check that.
         *
         * Device will not be initialized when the engine is in headless mode. Device::Get() will then throw an error.
         * Only check for IsHeadless if you find that Device::Get() is being called from somewhere during unit tests.
         */
        static bool IsHeadless() { return sIsHeadless; }

    //Platform specific heap
    public:
        std::shared_ptr<DXDescHeap> GetDescriptorHeap(int heap) { return mDescriptorHeaps[heap]; }
        int GetFrameIndex() { return mFrameIndex; }
 

    private:
        friend Engine;

        // Set to true by Engine if in unit_test mode.
        static inline bool sIsHeadless{};

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
        int mFrameBufferCount = RT_COUNT;
        int mDepthStencilCount = 1;
        std::vector<ComPtr<ID3D12Resource>> mResourcesToDeallocate;

        ComPtr<IDXGISwapChain3> mSwapChain;
        ComPtr<ID3D12Device5> mDevice;
        std::unique_ptr<DXSignature> mSignature;
        std::unique_ptr<DXSignature> mComputeSignature;
        std::unique_ptr<DXPipeline> mGenMipmapsPipeline;

        DXHeapHandle mRenderTargetHandles[FRAME_BUFFER_COUNT];
        DXHeapHandle mDepthHandle;

        bool mUpdateWindow = false;
    };
}


