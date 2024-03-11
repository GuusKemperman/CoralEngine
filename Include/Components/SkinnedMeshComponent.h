#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
    class SkinnedMesh;
    class Material;

    class SkinnedMeshComponent
    {
    public:   
        std::shared_ptr<const SkinnedMesh> mSkinnedMesh{};

        std::shared_ptr<const Material> mMaterial{};

    private:
        friend ReflectAccess;
        static MetaType Reflect();
        REFLECT_AT_START_UP(SkinnedMeshComponent);
    };
}