#pragma once
#ifdef PLATFORM_WINDOWS
#include "Platform/PC/Core/DevicePC.h"
#endif
//struct GLFWwindow;
//
//namespace Engine
//{
//    class Device final : 
//        public EngineSubsystem<Device>
//    {
//        friend EngineSubsystem;
//        Device();
//        ~Device() final override;
//
//    public:
//        void NewFrame();
//        void Render();
//
//        bool ShouldClose() const { return !mIsWindowOpen; }
//
//        GLFWwindow* mWindow;
//
//    private:
//        void InitializeImGui();
//
//        bool mIsWindowOpen{};
//    };
//}
