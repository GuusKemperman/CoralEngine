#pragma once
#include "Assets/Asset.h"
#include "xsr.hpp"

namespace Engine
{
    class Material;

    class StaticMesh final :
        public Asset
    {
    public:
	    StaticMesh(std::string_view name);
        StaticMesh(AssetLoadInfo& loadInfo);
        ~StaticMesh() override;

        StaticMesh(StaticMesh&& other) noexcept;
        StaticMesh(const StaticMesh&) = delete;

        StaticMesh& operator=(StaticMesh&&) = delete;
        StaticMesh& operator=(const StaticMesh&) = delete;

        const xsr::mesh_handle& GetHandle() const { return mMeshHandle; }
        void SetHandle(xsr::mesh_handle&& handle) { mMeshHandle = handle; }

    private:
        friend class StaticMeshImporter;

        // Returns true on success
        static bool OnSave(AssetSaveInfo& saveInfo,
            Span<const glm::vec3> positions,
            std::optional<std::variant<Span<const uint16>, Span<const uint32>>> indices,
            std::optional<Span<const glm::vec3>> normals,
            std::optional<Span<const glm::vec2>> textureCoordinates,
            std::optional<Span<const glm::vec3>> colors);
        
        xsr::mesh_handle mMeshHandle{};

        friend ReflectAccess;
        static MetaType Reflect();
        REFLECT_AT_START_UP(StaticMesh);
    };
}  // namespace Engine
