#pragma once

#include "Assets/Asset.h"
#include "Assets/Animation/BoneInfo.h"
#include "Utilities/Geometry3d.h"

class DXResource;

namespace CE
{
    class SkinnedMesh final :
        public Asset
    {
    public:
	    SkinnedMesh(std::string_view name);
        SkinnedMesh(AssetLoadInfo& loadInfo);
        ~SkinnedMesh() override {};

        SkinnedMesh(SkinnedMesh&& other) noexcept;
        SkinnedMesh(const SkinnedMesh&) = delete;

        void DrawMesh() const;
        void DrawMeshVertexOnly() const;

        const std::unordered_map<std::string, BoneInfo>& GetBoneMap() const { return mBoneInfoMap; };

        SkinnedMesh& operator=(SkinnedMesh&&) = delete;
        SkinnedMesh& operator=(const SkinnedMesh&) = delete;

#ifdef EDITOR
        // There is no reason why this NEEDS to be editor only,
        // feel free to remove all the ifdefs if you require this
        // data in non-editor builds. But since we likely won't need
        // it for non-editor builds and because it does take up additional
        // RAM, these buffers are, for now, editor only.
        Span<const glm::vec3> GetVertices() const { return mCPUVertexBuffer; }
        Span<const uint32> GetIndices() const { return mCPUIndexBuffer; }
        AABB3D GetBoundingBox() const { return mBoundingBox; }
#endif

    private:
        friend class ModelImporter;

        // Returns true on success
        static bool OnSave(AssetSaveInfo& saveInfo,
            Span<const glm::vec3> positions,
            std::optional<std::variant<Span<const uint16>, Span<const uint32>>> indices,
            std::optional<Span<const glm::vec3>> normals,
            std::optional<Span<const glm::vec3>> tangents,
            std::optional<Span<const glm::vec2>> textureCoordinates,
            std::optional<Span<const glm::ivec4>> boneIds,
            std::optional<Span<const glm::vec4>> boneWeights,
            std::optional<std::unordered_map<std::string, BoneInfo>> boneMap);
        
        friend ReflectAccess;
        static MetaType Reflect();
    private:
        bool LoadMesh(const char* indices, unsigned int indexCount, unsigned int size_of_index_type, const float* positions, const float* normalsBuffer, const float* textureCoordinates, const float* tangents, const int* boneIds, const float* boneWeights, unsigned int vertexCount);

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

        // Prevents having to include the very
		// large DX12 headers
        struct DXImpl;

        struct DXImplDeleter
        {
            void operator()(DXImpl* impl) const;
        };

        std::unique_ptr<DXImpl, DXImplDeleter> mImpl{};

        std::unordered_map<std::string, BoneInfo> mBoneInfoMap;
    };
}