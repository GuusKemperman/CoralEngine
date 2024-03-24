#pragma once
#include "Systems/System.h"

class DXPipeline;

namespace Engine
{
    class DebugRenderer;
    class World;

    class Renderer final :
        public System
    {
    public:
        Renderer();
        void Render(const World& world) override;

        SystemStaticTraits GetStaticTraits() const override
        {
            SystemStaticTraits traits{};
            traits.mPriority = static_cast<int>(TickPriorities::Render) + (TickPriorityStepSize >> 1);
            return traits;
        }

    private:
        std::unique_ptr<DXPipeline> mPBRPipeline;
        std::unique_ptr<DXPipeline> mPBRSkinnedPipeline;
        std::unique_ptr<DXPipeline> mZPipeline;
        std::unique_ptr<DXPipeline> mZSkinnedPipeline;
        
    private:
        friend ReflectAccess;
        static MetaType Reflect();
    };
}