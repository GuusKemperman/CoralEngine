#include "Precomp.h"
#include "assets/SkinnedMesh.h"

#include "Meta/MetaType.h"
#include "Utilities/StringFunctions.h"
#include "Utilities/Reflect/ReflectAssetType.h"
#include "Utilities/ClassVersion.h"
#include "Assets/Core/AssetSaveInfo.h"

#include <cereal/types/unordered_map.hpp>

namespace cereal
{
    inline void save(BinaryOutputArchive& ar, const Engine::BoneInfo& value)
    {
        ar(value.mOffset, value.mId);
    }

    inline void load(BinaryInputArchive& ar, Engine::BoneInfo& value)
    {
        ar(value.mOffset, value.mId);
    }
}

enum SkinnedMeshFlags : uint8
{
    hasIndices = 1,
    hasNormals = 1 << 1,
    hasUVs = 1 << 2,
    hasColors = 1 << 3, // No longer used
    areIndices16Bit = 1 << 4,
    hasTangents = 1 << 5,
    hasBoneIds = 1 << 6,
    hasBoneWeights = 1 << 7
};

Engine::SkinnedMesh::SkinnedMesh(std::string_view name) :
    Asset(name, MakeTypeId<SkinnedMesh>())
{}

bool Engine::SkinnedMesh::OnSave(AssetSaveInfo& saveInfo,
    Span<const glm::vec3> positions, 
    std::optional<std::variant<Span<const uint16>, Span<const uint32>>> indices, 
    std::optional<Span<const glm::vec3>> normals, 
    std::optional<Span<const glm::vec3>> tangents, 
    std::optional<Span<const glm::vec2>> uvs, 
    std::optional<Span<const glm::ivec4>> boneIds, 
    std::optional<Span<const glm::vec4>> boneWeights,
    std::optional<std::unordered_map<std::string, BoneInfo>> boneMap)
{
    const uint32 numOfIndices = indices.has_value() ? (static_cast<uint32>(std::holds_alternative<Span<const uint16>>(*indices) ?
        std::get<Span<const uint16>>(*indices).size() :
        std::get<Span<const uint32>>(*indices).size())) : 0;

    if (numOfIndices % 3 != 0)
    {
        LOG(LogAssets, Error, "Importing static mesh failed: {} indices provided, but this is not divisible by 3", numOfIndices);
        return false;
    }

    if (normals.has_value()
        && positions.size() != normals->size())
    {
        LOG(LogAssets, Error, "Importing static mesh failed: expected {} normals, but received {}", positions.size(), normals->size());
        return false;
    }

    if (uvs.has_value()
        && positions.size() != uvs->size())
    {
        LOG(LogAssets, Error, "Importing static mesh failed: expected {} textureCoordinates, but received {}", positions.size(), uvs->size());
        return false;
    }

    if (tangents.has_value()
        && positions.size() != tangents->size())
    {
        LOG(LogAssets, Error, "Importing static mesh failed: expected {} tangents, but received {}", positions.size(), tangents->size());
        return false;
    }

    if (boneIds.has_value()
        && positions.size() != boneIds->size())
    {
        LOG(LogAssets, Error, "Importing static mesh failed: expected {} bone ids, but received {}", positions.size(), boneIds->size());
        return false;
    }

    if (boneWeights.has_value()
        && positions.size() != boneWeights->size())
    {
        LOG(LogAssets, Error, "Importing static mesh failed: expected {} bone weights, but received {}", positions.size(), boneWeights->size());
        return false;
    }

    std::ostream& str = saveInfo.GetStream();

    const uint32 numOfPositions = static_cast<uint32>(positions.size());

    SkinnedMeshFlags flags{};

    if (indices.has_value())
    {
        flags = static_cast<SkinnedMeshFlags>(flags | hasIndices);

        if (numOfPositions < std::numeric_limits<uint16>::max()
            || std::holds_alternative<Span<const uint16>>(*indices))
        {
            flags = static_cast<SkinnedMeshFlags>(flags | areIndices16Bit);
        }
    }

    if (normals.has_value()) flags = static_cast<SkinnedMeshFlags>(flags | hasNormals);
    if (uvs.has_value()) flags = static_cast<SkinnedMeshFlags>(flags | hasUVs);
    if (tangents.has_value()) flags = static_cast<SkinnedMeshFlags>(flags | hasTangents);
    if (boneIds.has_value()) flags = static_cast<SkinnedMeshFlags>(flags | hasBoneIds);
    if (boneWeights.has_value()) flags = static_cast<SkinnedMeshFlags>(flags | hasBoneWeights);

    str.write(reinterpret_cast<const char*>(&flags), sizeof(SkinnedMeshFlags));

    str.write(reinterpret_cast<const char*>(&numOfPositions), sizeof(numOfPositions));
    str.write(reinterpret_cast<const char*>(positions.data()), positions.size_bytes());

    if (indices.has_value())
    {
        str.write(reinterpret_cast<const char*>(&numOfIndices), sizeof(numOfIndices));

        if (std::holds_alternative<Span<const uint16>>(*indices))
        {
            const Span<const uint16>& shorts = std::get<Span<const uint16>>(*indices);
            str.write(reinterpret_cast<const char*>(shorts.data()), shorts.size_bytes());
            ASSERT(flags & areIndices16Bit);
        }
        else
        {
            const Span<const uint32>& unsigneds = std::get<Span<const uint32>>(*indices);

            if (numOfPositions >= std::numeric_limits<uint16>::max())
            {
                str.write(reinterpret_cast<const char*>(unsigneds.data()), unsigneds.size_bytes());
                ASSERT((flags & areIndices16Bit) == 0);
            }
            else
            {
                const std::vector<uint16> shorts = { unsigneds.data(), unsigneds.data() + unsigneds.size() };
                str.write(reinterpret_cast<const char*>(shorts.data()), shorts.size() * sizeof(uint16));
                ASSERT(flags & areIndices16Bit);
            }
        }
    }
    if (normals.has_value()) str.write(reinterpret_cast<const char*>(normals->data()), normals->size_bytes());
    if (uvs.has_value()) str.write(reinterpret_cast<const char*>(uvs->data()), uvs->size_bytes());
    if (tangents.has_value()) str.write(reinterpret_cast<const char*>(tangents->data()), tangents->size_bytes());
    if (boneIds.has_value()) str.write(reinterpret_cast<const char*>(boneIds->data()), boneIds->size_bytes());
    if (boneWeights.has_value()) str.write(reinterpret_cast<const char*>(boneWeights->data()), boneWeights->size_bytes());

    BinaryGSONObject obj { "SkinnedMesh" }; 

    obj.AddGSONMember("BoneMap") << boneMap.value();

    obj.SaveToBinary(str);

    return true;
}

Engine::MetaType Engine::SkinnedMesh::Reflect()
{
    MetaType type = MetaType{ MetaType::T<SkinnedMesh>{}, "SkinnedMesh", MetaType::Base<Asset>{}, MetaType::Ctor<AssetLoadInfo&>{}, MetaType::Ctor<std::string_view>{} };

    SetClassVersion(type, 1);

    ReflectAssetType<SkinnedMesh>(type);
    return type;
}