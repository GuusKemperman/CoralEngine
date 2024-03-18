#pragma once
#include "DX12Classes/DXDefines.h"
#include "Meta/MetaReflect.h"
#include "Systems/System.h"
#include "Platform/PC/Rendering/InfoStruct.h"

class DXResource;
class DXConstBuffer;
class DXPipeline;
class DXSignature;
class DXDescHeap;

namespace Engine
{
    class World;
    class CameraComponent;

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
        void CalculateClusterGrid(const CameraComponent& camera);

        std::unique_ptr<DXConstBuffer> mConstBuffers[NUM_CBS];
        std::unique_ptr<DXPipeline> mPBRPipeline;
        std::unique_ptr<DXPipeline> mClusterGridPipeline;
        std::unique_ptr<DXPipeline> mZPipeline;
        InfoStruct::DXLightInfo mLights;

        std::unique_ptr<DXResource> mClusterResource;
        unsigned int mClusterUavIndex = 0;
        glm::vec2 screenSize = glm::vec2(1.f, 1.f);
        bool updateClusterGrid = false;
        
    private:
        friend ReflectAccess;
        static MetaType Reflect();
    };
}