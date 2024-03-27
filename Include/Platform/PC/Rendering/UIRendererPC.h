#pragma once
#include "Rendering/ISubRenderer.h"

namespace Engine
{
    class UIRenderer final :
        public ISubRenderer
    {
    public:
        UIRenderer();
        ~UIRenderer();
        void Render(const World& world) override;
    };
}