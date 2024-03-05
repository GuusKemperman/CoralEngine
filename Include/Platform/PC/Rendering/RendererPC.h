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
            float padding1; // Padding to align the next bools

            // Grouping bools together for efficient packing, and adding padding to ensure
            // the struct ends on a 16-byte boundary. Even though individual bools are treated
            // as 4 bytes in HLSL, aligning them like this without additional padding will not align
            // the struct size since the total size becomes 60 bytes, which is not a multiple of 16.
            bool useColorTex;
            bool useEmissiveTex;
            bool useMetallicRoughnessTex;
            bool useNormalTex;
            bool useOcclusionTex;
            char padding2[11]; // Additional padding to align the struct size to a multiple of 16 bytes        
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

    private:
        std::unique_ptr<DXConstBuffer> mConstBuffers[NUM_CBS];
        std::unique_ptr<DXPipeline> mPipelines[NUM_PIPELINES];
        std::unique_ptr<DXSignature> mSignature;

        InfoStruct::DXLightInfo  lights;
        std::vector<glm::mat4x4> modelMatrices;

    private:
        friend ReflectAccess;
        static MetaType Reflect();
        REFLECT_AT_START_UP(Renderer);
    };
}