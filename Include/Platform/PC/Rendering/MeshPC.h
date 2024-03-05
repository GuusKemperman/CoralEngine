#pragma once
#include <optional>
#include <variant>

#include "DX12Classes/DXDefines.h"
#include "Assets/Asset.h"

class DXResource;

namespace Engine
{
    class Material;

    class StaticMesh final :
        public Asset
    {
    public:
        StaticMesh(std::string_view name);
        StaticMesh(AssetLoadInfo& loadInfo);
        ~StaticMesh() override {};

        StaticMesh(StaticMesh&& other) noexcept;
        StaticMesh(const StaticMesh&) = delete;

        void DrawMesh() const;

        StaticMesh& operator=(StaticMesh&&) = delete;
        StaticMesh& operator=(const StaticMesh&) = delete;

    private:
        friend class StaticMeshImporter;

        // Returns true on success
        static bool OnSave(AssetSaveInfo& saveInfo,
            Span<const glm::vec3> positions,
            std::optional<std::variant<Span<const uint16>, Span<const uint32>>> indices,
            std::optional<Span<const glm::vec3>> normals,
            std::optional<Span<const glm::vec3>> tangents,
            std::optional<Span<const glm::vec2>> textureCoordinates);

        friend ReflectAccess;
        static MetaType Reflect();

    private:
        bool LoadMesh(const char* indices, unsigned int indexCount, unsigned int size_of_index_type, const float* positions, const float* normalsBuffer, const float* textureCoordinates, const float* tangents, unsigned int vertexCount);

        std::shared_ptr<DXResource> mVertexBuffer;
        std::shared_ptr<DXResource> mNormalBuffer;
        std::shared_ptr<DXResource> mTangentBuffer;
        std::shared_ptr<DXResource> mTexCoordBuffer;
        std::shared_ptr<DXResource> mIndexBuffer;

        D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
        D3D12_VERTEX_BUFFER_VIEW mNormalBufferView;
        D3D12_VERTEX_BUFFER_VIEW mTexCoordBufferView;
        D3D12_VERTEX_BUFFER_VIEW mTangentBufferView;
        D3D12_INDEX_BUFFER_VIEW mIndexBufferView;

        int mIndexCount = 0;
        int mVertexCount = 0;
        DXGI_FORMAT mIndexFormat;
        bool beenUpdated = false;

    };
}  // namespace Engine
