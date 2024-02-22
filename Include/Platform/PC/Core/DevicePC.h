#pragma once
#include "Core/EngineSubsystem.h"
#include "Platform/PC/Rendering/DXDefines.h"
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
    namespace InfoStruct {
        struct DXMatrixInfo {
            glm::mat4x4 vm;
            glm::mat4x4 pm;
            glm::mat4x4 ivm;
            glm::mat4x4 ipm;
        };

        struct DXDirLightInfo {
            glm::vec4 dir = { 0.f, 0.0f, 0.0f, 0.f };
            glm::vec4 colorAndIntensity = { 0.f, 0.0f, 0.0f, 0.f };
        };

        struct DXPointLightInfo {
            glm::vec4 position = { 0.f, 0.0f, 0.0f, 0.f };
            glm::vec4 colorAndIntensity = { 0.f, 0.0f, 0.0f, 0.f };
            float radius = 0.f;
            float padding[3];
        };

        struct DXLightInfo {
            DXPointLightInfo pointLights[MAX_LIGHTS];
            DXDirLightInfo dirLights[MAX_LIGHTS];
        };

        struct DXMaterialInfo
        {
            bool useColorTex;
            bool useEmissiveTex;
            bool useMetallicRoughnessTex;
            bool useNormalTex;
            bool useOcclusionTex;
            bool padding1;
            bool padding2;
            bool padding3;

            glm::vec4 colorFactor;
            glm::vec4 emissiveFactor;
            float metallicFactor;
            float roughnessFactor;
            float normalScale;
            float padding4;
        };

    }

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

        HANDLE mFenceEvent;
        UINT64 mFenceValue[FRAME_BUFFER_COUNT];

        ComPtr<IDXGISwapChain3> mSwapChain;
        ComPtr<ID3D12Device5> mDevice;
        std::unique_ptr<DXDescHeap> imguiHeap;
    };
}


