#pragma once
#include "Core/EngineSubsystem.h"

#ifdef PLATFORM_WINDOWS

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

    	void CreateImguiContext();

    	GLFWwindow* mWindow;

    private:

        bool mIsWindowOpen{};
    };
}

#elif PLATFORM_***REMOVED***

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

        bool ShouldClose() const { return false; }

        void CreateImguiContext();
    };
}

#endif