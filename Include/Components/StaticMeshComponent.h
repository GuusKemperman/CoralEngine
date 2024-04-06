#pragma once
#include "Meta/MetaReflect.h"

namespace CE
{
    class StaticMesh;
    class Material;

    class StaticMeshComponent
    {
    public:   
        std::shared_ptr<const StaticMesh> mStaticMesh{};

        std::shared_ptr<const Material> mMaterial{};

        bool mTilesWithMeshScale = false;

        float mTiling = 1;

    private:
        friend ReflectAccess;
        static MetaType Reflect();
        REFLECT_AT_START_UP(StaticMeshComponent);
    };
}