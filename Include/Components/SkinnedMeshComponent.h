#pragma once
#include "Assets/Core/AssetHandle.h"
#include "Meta/MetaReflect.h"
#include "Rendering/SkinnedMeshDefines.h"

namespace CE
{
    class SkinnedMesh;
    class Animation;
    class Material;

    class SkinnedMeshComponent
    {
    public:   
        AssetHandle<SkinnedMesh> mSkinnedMesh{};

        AssetHandle<Animation> mAnimation{};

        AssetHandle<Material> mMaterial{};

        std::vector<glm::mat4x4> mFinalBoneMatrices{MAX_BONES, glm::mat4x4(1.0f)};
        float mCurrentTime = 0.0f;
        float mAnimationSpeed = 1.0f;

    private:
        friend ReflectAccess;
        static MetaType Reflect();
        REFLECT_AT_START_UP(SkinnedMeshComponent);
    };
}