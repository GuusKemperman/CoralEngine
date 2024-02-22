#pragma once
#include "Core/EngineSubsystem.h"
#include "Meta/MetaReflect.h"
#include "Systems/System.h"
#include "DXDefines.h"

#pragma warning(push)
#pragma warning(disable: 4005)
#include <wrl.h>
#define NOMINMAX
#include <Windows.h>
#pragma warning(pop) 
using namespace Microsoft::WRL;

class DXResource;
class DXConstBuffer;
class DXPipeline;
class DXSignature;
class DXDescHeap;

namespace Engine
{
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
        // also could be some platform specific variables here and functions which you want to add

    private:
        friend ReflectAccess;
        static MetaType Reflect();
        REFLECT_AT_START_UP(Renderer);

        std::unique_ptr<DXDescHeap> mDescriptorHeaps[NUM_DESC_HEAPS];
        std::unique_ptr<DXResource> mResources[NUM_RESOURCES];
        std::unique_ptr<DXConstBuffer> mConstBuffers[NUM_CBS];
        std::unique_ptr<DXPipeline> mPipelines[NUM_PIPELINES];
        std::unique_ptr<DXSignature> mSignature;

        const DXGI_FORMAT mDepthFormat = DXGI_FORMAT_D32_FLOAT;
    };
}