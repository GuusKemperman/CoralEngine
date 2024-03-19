#pragma once

#include "DX12Classes/DXDefines.h"
#include "Assets/Asset.h"
#include "components/Animation/BoneInfo.h"

class DXResource;

namespace Engine
{
    #define MAX_BONES 128
    #define MAX_BONE_INFLUENCE 4

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

        const std::unordered_map<std::string, BoneInfo>& GetBoneMap() const { return mBoneInfoMap; };

        SkinnedMesh& operator=(SkinnedMesh&&) = delete;
        SkinnedMesh& operator=(const SkinnedMesh&) = delete;

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

        std::shared_ptr<DXResource> mVertexBuffer;
        std::shared_ptr<DXResource> mNormalBuffer;
        std::shared_ptr<DXResource> mTangentBuffer;
        std::shared_ptr<DXResource> mTexCoordBuffer;
        std::shared_ptr<DXResource> mIndexBuffer;
        std::shared_ptr<DXResource> mBoneIdBuffer;
        std::shared_ptr<DXResource> mBoneWeightBuffer;

        D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
        D3D12_VERTEX_BUFFER_VIEW mNormalBufferView;
        D3D12_VERTEX_BUFFER_VIEW mTexCoordBufferView;
        D3D12_VERTEX_BUFFER_VIEW mTangentBufferView;
        D3D12_VERTEX_BUFFER_VIEW mBoneIdBufferView;
        D3D12_VERTEX_BUFFER_VIEW mBoneWeightBufferView;
        D3D12_INDEX_BUFFER_VIEW mIndexBufferView;

        std::unordered_map<std::string, BoneInfo> mBoneInfoMap;

        int mIndexCount = 0;
        int mVertexCount = 0;
        DXGI_FORMAT mIndexFormat;
        bool mBeenUpdated = false;
    };
}