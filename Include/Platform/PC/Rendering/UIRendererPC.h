#pragma once

#include "Systems/System.h"

namespace Engine
{
    class UIRenderer final :
        public System
    {
    public:
        UIRenderer();
        ~UIRenderer() override;
        void Render(const World& world) override;

        SystemStaticTraits GetStaticTraits() const override
        {
            SystemStaticTraits traits{};
            traits.mPriority = static_cast<int>(TickPriorities::PostRender) + (TickPriorityStepSize >> 1);
            return traits;
        }

    private:
        friend ReflectAccess;
        static MetaType Reflect();
    };
}