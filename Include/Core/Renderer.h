#pragma once
#include "Core/EngineSubsystem.h"

struct GLFWwindow;

namespace Engine
{
    class Renderer final : 
        public EngineSubsystem<Renderer>
    {
        friend EngineSubsystem;
        Renderer();
        ~Renderer() final override;

    public:
        void NewFrame();
        void Render();

        bool ShouldClose() const { return !mIsWindowOpen; }

        GLFWwindow* mWindow;

    private:
        void CreateImguiContext();

        bool mIsWindowOpen{};
    };
}
