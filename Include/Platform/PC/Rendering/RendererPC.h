#pragma once
#include "DX12Classes/DXDefines.h"
#include "Core/EngineSubsystem.h"
#include "Meta/MetaReflect.h"
#include "Systems/System.h"
#include "glm/glm.hpp"
#include "Assets/StaticMesh.h"

class DXResource;
class DXConstBuffer;
class DXPipeline;
class DXSignature;
class DXDescHeap;

namespace Engine
{
    class DebugRenderer;
    class World;
    namespace InfoStruct {
        struct DXMatrixInfo {
            glm::mat4x4 vm;
            glm::mat4x4 pm;
            glm::mat4x4 ivm;
            glm::mat4x4 ipm;
        };

        struct DXDirLightInfo {
            glm::vec4 mDir = { 0.f, 0.0f, 0.0f, 0.f };
            glm::vec4 mColorAndIntensity = { 0.f, 0.0f, 0.0f, 0.f };
        };

        struct DXPointLightInfo {
            glm::vec4 mPosition = { 0.f, 0.0f, 0.0f, 0.f };
            glm::vec4 mColorAndIntensity = { 0.f, 0.0f, 0.0f, 0.f };
            float mRadius = 0.f;
            float padding[3];
        };

        struct DXLightInfo {
            DXPointLightInfo mPointLights[MAX_LIGHTS];
            DXDirLightInfo mDirLights[MAX_LIGHTS];
        };

        struct DXMaterialInfo
        {
            glm::vec4 colorFactor;
            glm::vec4 emissiveFactor;
            float metallicFactor;
            float roughnessFactor;
            float normalScale;
            unsigned int useColorTex;
            unsigned int useEmissiveTex;
            unsigned int useMetallicRoughnessTex;
            unsigned int useNormalTex;
            unsigned int useOcclusionTex;
        };
    }


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

        enum DXStructuredBuffers {
            MODEL_MAT_SB,
            MATERIAL_SB
        };


    private:
        std::unique_ptr<DXConstBuffer> mConstBuffers[NUM_CBS];
        std::unique_ptr<DXResource> mStructuredBuffers[3];
        std::unique_ptr<DXPipeline> mPBRPipeline;
        std::unique_ptr<DXPipeline> mZPipeline;
        InfoStruct::DXLightInfo  lights;

        unsigned int materialHeapSlot = 0;
        std::vector<InfoStruct::DXMaterialInfo> materials;
    private:
        friend ReflectAccess;
        static MetaType Reflect();
    };
}