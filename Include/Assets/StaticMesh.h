#pragma once
#include "Asset.h"
#include "Utilities/Geometry3d.h"

namespace CE
{
	struct StaticMeshPlatformImpl;

	class StaticMesh :
		public Asset
	{
	public:
        StaticMesh(std::string_view name);
        StaticMesh(AssetLoadInfo& loadInfo);

#ifdef EDITOR
        // There is no reason why this NEEDS to be editor only,
        // feel free to remove all the ifdefs if you require this
        // data in non-editor builds. But since we likely won't need
        // it for non-editor builds and because it does take up additional
        // RAM, these buffers are, for now, editor only.
        std::span<const glm::vec3> GetVertices() const { return mCPUVertexBuffer; }
        std::span<const uint32> GetIndices() const { return mCPUIndexBuffer; }
        AABB3D GetBoundingBox() const { return mBoundingBox; }
#endif

        const std::shared_ptr<StaticMeshPlatformImpl>& GetPlatformImpl() const { return mImpl; }

    private:
        friend ReflectAccess;
        static MetaType Reflect();
        REFLECT_AT_START_UP(StaticMesh);

        std::shared_ptr<StaticMeshPlatformImpl> mImpl{};

#ifdef EDITOR
        // There is no reason why this NEEDS to be editor only,
        // feel free to remove all the ifdefs if you require this
        // data in non-editor builds. But since we likely won't need
        // it for non-editor builds and because it does take up additional
        // RAM, these buffers are, for now, editor only.
        std::vector<glm::vec3> mCPUVertexBuffer{};
        std::vector<uint32> mCPUIndexBuffer{};
        AABB3D mBoundingBox{};
#endif
	};

    namespace Internal
    {
        // Returns true on success
        bool SaveStaticMesh(AssetSaveInfo& saveInfo,
            std::span<const glm::vec3> positions,
            std::span<const uint32> indices,
            std::span<const glm::vec3> normals,
            std::span<const glm::vec3> tangents,
            std::span<const glm::vec2> textureCoordinates);
    }
}
