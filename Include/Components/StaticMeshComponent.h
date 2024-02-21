#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
    class StaticMesh;
    class Material;

    class StaticMeshComponent
    {
    public:   
        std::shared_ptr<const StaticMesh> mStaticMesh{};

        std::shared_ptr<const Material> mMaterial{};

    private:
        friend ReflectAccess;
        static MetaType Reflect();
        REFLECT_AT_START_UP(StaticMeshComponent);
    };
}