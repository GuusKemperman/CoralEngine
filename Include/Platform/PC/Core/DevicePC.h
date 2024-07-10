#pragma once
#include "Core/EngineSubsystem.h"

struct GLFWwindow;
struct GLFWmonitor;
class DXDescHeap;

namespace Microsoft::WRL
{
    template <typename T>
    class ComPtr;
}

struct ID3D12Resource;

namespace CE
{
    class Engine;
    class FrameBuffer;

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

        void* GetWindow();
        void* GetSwapchain();
        void* GetDevice();
        void* GetCommandList();
        void* GetUploadCommandList();
        void* GetCommandQueue();
        void* GetSignature();
        void* GetComputeSignature();
        void* GetMipmapPipeline();

#ifdef EDITOR
        void CreateImguiContext();
#endif // EDITOR

		bool StartUploadCommands();
        void SubmitUploadCommands();
        void AddToDeallocation(Microsoft::WRL::ComPtr<ID3D12Resource>&& res);
        void BindSwapchainRT();
        void ResolveMsaa(FrameBuffer & msaaFramebuffer);
        void CopyToRenderTargets(FrameBuffer& source);
        glm::vec2 GetDisplaySize();
        glm::vec2 GetWindowPosition();

        /**
         * \brief Some build/testing servers do not support graphics. This function can be used to check that.
         *
         * Device will not be initialized when the engine is in headless mode. Device::Get() will then throw an error.
         * Only check for IsHeadless if you find that Device::Get() is being called from somewhere during unit tests.
         */
        static bool IsHeadless() { return sIsHeadless; }

		//Platform specific heap
        std::shared_ptr<DXDescHeap> GetDescriptorHeap(int heap);
        int GetFrameIndex();

    private:
        friend Engine;

        // Set to true by Engine if in unit_test mode.
        static inline bool sIsHeadless{};

        void InitializeWindow();
        void InitializeDevice();
        void UpdateRenderTarget();
        void SubmitCommands();
        void StartRecordingCommands();

        void SendTexturesToGPU();

        // Prevents having to include the very
		// large DX12 headers
        struct DXImpl;

        struct DXImplDeleter
        {
            void operator()(DXImpl* impl) const;
        };

        std::unique_ptr<DXImpl, DXImplDeleter> mImpl{};

        bool mIsWindowOpen{};

        GLFWwindow* mWindow{};
        GLFWmonitor* mMonitor{};

#ifdef EDITOR
        bool mFullscreen = false;
#else
        bool mFullscreen = true;
#endif

        int mPreviousWidth{};
		int mPreviousHeight{};

        int mPreviousPosX{};
        int mPreviousPosY{};

        bool mUpdateWindow = false;
        bool mUploadCommandListOpen = false;
    };
}
