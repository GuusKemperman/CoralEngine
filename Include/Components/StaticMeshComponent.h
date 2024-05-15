#pragma once
#include "Assets/Core/AssetHandle.h"
#include "Meta/MetaReflect.h"

namespace CE
{
    class StaticMesh;
    class Material;

    class StaticMeshComponent
    {
    public:   
        AssetHandle<StaticMesh> mStaticMesh{};

        AssetHandle<Material> mMaterial{};

        bool mTilesWithMeshScale = false;

        float mTiling = 1;

        bool mHighlightedMesh = false;

    private:
        friend ReflectAccess;
        static MetaType Reflect();
        REFLECT_AT_START_UP(StaticMeshComponent);
    };
}