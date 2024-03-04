#pragma once
#include "DX12Classes/DXDefines.h"
#include "Core/EngineSubsystem.h"
#include "Meta/MetaReflect.h"
#include "Systems/System.h"
#include "glm/glm.hpp"

class DXResource;
class DXConstBuffer;
class DXPipeline;
class DXSignature;
class DXDescHeap;

namespace Engine
{
    class World;
    namespace InfoStruct {
        struct DXMatrixInfo {
            glm::mat4x4 vm;
            glm::mat4x4 pm;
            glm::mat4x4 ivm;
            glm::mat4x4 ipm;
        };

        struct DXDirLightInfo {
            glm::vec4 dir = { 0.f, 0.0f, 0.0f, 0.f };
            glm::vec4 colorAndIntensity = { 0.f, 0.0f, 0.0f, 0.f };
        };

        struct DXPointLightInfo {
            glm::vec4 position = { 0.f, 0.0f, 0.0f, 0.f };
            glm::vec4 colorAndIntensity = { 0.f, 0.0f, 0.0f, 0.f };
            float radius = 0.f;
            float padding[3];
        };

        struct DXLightInfo {
            DXPointLightInfo pointLights[MAX_LIGHTS];
            DXDirLightInfo dirLights[MAX_LIGHTS];
        };

        struct DXMaterialInfo
        {
            bool useColorTex;
            bool useEmissiveTex;
            bool useMetallicRoughnessTex;
            bool useNormalTex;
            bool useOcclusionTex;
            bool padding1;
            bool padding2;
            bool padding3;

            glm::vec4 colorFactor;
            glm::vec4 emissiveFactor;
            float metallicFactor;
            float roughnessFactor;
            float normalScale;
            float padding4;
        };
    }

    class RenderToCamerasSystem final :
        public System
    {
    public:
        RenderToCamerasSystem();
        void Render(const World& world) override;

        SystemStaticTraits GetStaticTraits() const override
        {
            SystemStaticTraits traits{};
            traits.mPriority = static_cast<int>(TickPriorities::Render) + (TickPriorityStepSize >> 1);
            return traits;
        }

    private:
        std::unique_ptr<DXConstBuffer> mConstBuffers[NUM_CBS];
        std::unique_ptr<DXPipeline> mPipelines[NUM_PIPELINES];
        std::unique_ptr<DXSignature> mSignature;

        InfoStruct::DXLightInfo  lights;
        std::vector<glm::mat4x4> modelMatrices;

    private:
        friend ReflectAccess;
        static MetaType Reflect();
        REFLECT_AT_START_UP(RenderToCamerasSystem);
    };
}