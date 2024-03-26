#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
    class SkinnedMesh;
    class Animation;
    class Material;

    class SkinnedMeshComponent
    {
    public:   
        std::shared_ptr<const SkinnedMesh> mSkinnedMesh{};

        std::shared_ptr<const Animation> mAnimation{};

        std::shared_ptr<const Material> mMaterial{};

        std::vector<glm::mat4x4> mFinalBoneMatrices{128, glm::mat4x4(1.0f)};
        float mCurrentTime = 0.0f;

    private:
        friend ReflectAccess;
        static MetaType Reflect();
        REFLECT_AT_START_UP(SkinnedMeshComponent);
    };
}