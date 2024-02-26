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

        // TODO hardcoded return value
        glm::vec2 GetDisplaySize() const;

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

        // TODO hardcoded return value
        glm::vec2 GetDisplaySize() const { return { 1920.0f, 1080.0f }; }

        void CreateImguiContext();
    };
}

#endif