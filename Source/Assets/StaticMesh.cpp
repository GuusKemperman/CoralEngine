//#include "Precomp.h"
//#include "Assets/StaticMesh.h"
//
//#include <numeric> 
//
//#include "Utilities/StringFunctions.h"
//#include "Utilities/Reflect/ReflectAssetType.h"
//#include "Meta/MetaManager.h"
//#include "Assets/Core/AssetLoadInfo.h"
//#include "Assets/Core/AssetSaveInfo.h"
//#include "MeshPC.h"

//enum StaticMeshFlags : uint8
//{
//    hasIndices = 1,
//    hasNormals = 1 << 1,
//    hasUVs = 1 << 2,
//    hasColors = 1 << 3,
//    areIndices16Bit = 1 << 4,
//};
//
//Engine::StaticMesh::StaticMesh(std::string_view name) :
//    Asset(name, MakeTypeId<StaticMesh>())
//{
//}
//
//Engine::StaticMesh::StaticMesh(AssetLoadInfo& loadInfo) :
//	Asset(loadInfo)
//{
//    std::istream& str = loadInfo.GetStream();
//
//    StaticMeshFlags flags{};
//    str.read(reinterpret_cast<char*>(&flags), sizeof(flags));
//
//    uint32 numOfVertices{};
//    str.read(reinterpret_cast<char*>(&numOfVertices), sizeof(numOfVertices));
//
//    std::vector<glm::vec3> positions(numOfVertices);
//    str.read(reinterpret_cast<char*>(positions.data()), numOfVertices * sizeof(glm::vec3));
//
//    std::vector<char> indices{};
//    const uint32 indicesSizeOfType = flags & areIndices16Bit ? sizeof(uint16) : sizeof(uint32);
//
//    uint32 numOfIndices{};
//
//    if (flags & hasIndices)
//    {
//        str.read(reinterpret_cast<char*>(&numOfIndices), sizeof(numOfIndices));
//
//        indices.resize(numOfIndices * indicesSizeOfType);
//        str.read(reinterpret_cast<char*>(indices.data()), numOfIndices * indicesSizeOfType);
//    }
//    else
//    {
//        numOfIndices = static_cast<uint32>(positions.size());
//        indices.resize(positions.size());
//        std::iota(indices.begin(), indices.end(), 1);
//    }
//
//    std::vector<glm::vec3> normalsStorage(0);
//    const glm::vec3* normals = nullptr;
//
//    if (flags & hasNormals)
//    {
//        normalsStorage.resize(numOfVertices);
//        str.read(reinterpret_cast<char*>(normalsStorage.data()), numOfVertices * sizeof(glm::vec3));
//        normals = normalsStorage.data();
//    }
//
//    std::vector<glm::vec2> UVsStorage(0);
//    const glm::vec2* UVs = nullptr;
//
//    if (flags & hasUVs)
//    {
//        UVsStorage.resize(numOfVertices);
//        str.read(reinterpret_cast<char*>(UVsStorage.data()), numOfVertices * sizeof(glm::vec2));
//        UVs = UVsStorage.data();
//    }
//
//    std::vector<glm::vec3> colorsStorage(0);
//    const glm::vec3* colors = nullptr;
//
//    if (flags & hasColors)
//    {
//        colorsStorage.resize(numOfVertices);
//        str.read(reinterpret_cast<char*>(colorsStorage.data()), numOfVertices * sizeof(glm::vec3));
//        colors = colorsStorage.data();
//    }
//
//    mMeshHandle = xsr::create_mesh(indices.data(),
//        numOfIndices,
//        indicesSizeOfType,
//        reinterpret_cast<const float*>(positions.data()),
//        reinterpret_cast<const float*>(normals),
//        reinterpret_cast<const float*>(UVs),
//        reinterpret_cast<const float*>(colors),
//        numOfVertices);
//
//    if (!mMeshHandle.is_valid())
//    {
//        LOG(LogAssets, Error, "Loading of {} failed: Invalid mesh", GetName());
//    }
//}
//
//Engine::StaticMesh::~StaticMesh()
//{
//	free_mesh(mMeshHandle);
//}
//
//Engine::StaticMesh::StaticMesh(StaticMesh&& other) noexcept :
//	Asset(std::move(other))
//{
//    mMeshHandle = other.mMeshHandle;
//    other.mMeshHandle.id = 0;
//}
//
//bool Engine::StaticMesh::OnSave(AssetSaveInfo& saveInfo, 
//                                Span<const glm::vec3> positions, 
//                                std::optional<std::variant<Span<const uint16>, Span<const uint32>>> indices,
//                                std::optional<Span<const glm::vec3>> normals, 
//                                std::optional<Span<const glm::vec2>> uvs, 
//                                std::optional<Span<const glm::vec3>> colors)
//{
//    const uint32 numOfIndices = indices.has_value() ? (static_cast<uint32>(std::holds_alternative<Span<const uint16>>(*indices) ?
//        std::get<Span<const uint16>>(*indices).size() :
//        std::get<Span<const uint32>>(*indices).size())) : 0;
//
//    if (numOfIndices % 3 != 0)
//    {
//        LOG(LogAssets, Error, "Importing static mesh failed: {} indices provided, but this is not divisible by 3", numOfIndices);
//        return false;   
//    }
//    
//    if (normals.has_value()
//        && positions.size() != normals->size())
//    {
//        LOG(LogAssets, Error, "Importing static mesh failed: expected {} normals, but received {}", positions.size(), normals->size());
//        return false;
//    }
//
//    if (uvs.has_value()
//        && positions.size() != uvs->size())
//    {
//        LOG(LogAssets, Error, "Importing static mesh failed: expected {} textureCoordinates, but received {}", positions.size(), uvs->size());
//        return false;
//    }
//
//    if (colors.has_value()
//        && positions.size() != colors->size())
//    {
//        LOG(LogAssets, Error, "Importing static mesh failed: expected {} vertex colors, but received {}", positions.size(), colors->size());
//        return false;
//    }
//
//    std::ostream& str = saveInfo.GetStream();
//
//    const uint32 numOfPositions = static_cast<uint32>(positions.size());
//
//    StaticMeshFlags flags{};
//    
//    if (indices.has_value())
//    {
//        flags = static_cast<StaticMeshFlags>(flags | hasIndices);
//
//        if (numOfPositions < std::numeric_limits<uint16>::max()
//            || std::holds_alternative<Span<const uint16>>(*indices))
//        {
//            flags = static_cast<StaticMeshFlags>(flags | areIndices16Bit);
//        }
//    }
//
//    if (normals.has_value()) flags = static_cast<StaticMeshFlags>(flags | hasNormals);
//    if (uvs.has_value()) flags = static_cast<StaticMeshFlags>(flags | hasUVs);
//    if (colors.has_value()) flags = static_cast<StaticMeshFlags>(flags | hasColors);
//
//    str.write(reinterpret_cast<const char*>(&flags), sizeof(StaticMeshFlags));
//
//    str.write(reinterpret_cast<const char*>(&numOfPositions), sizeof(numOfPositions));
//    str.write(reinterpret_cast<const char*>(positions.data()), positions.size_bytes());
//
//    if (indices.has_value())
//    {
//        str.write(reinterpret_cast<const char*>(&numOfIndices), sizeof(numOfIndices));
//
//        if (std::holds_alternative<Span<const uint16>>(*indices))
//        {
//            const Span<const uint16>& shorts = std::get<Span<const uint16>>(*indices);
//            str.write(reinterpret_cast<const char*>(shorts.data()), shorts.size_bytes());
//            ASSERT(flags & areIndices16Bit);
//        }
//        else
//        {
//            const Span<const uint32>& unsigneds = std::get<Span<const uint32>>(*indices);
//
//            if (numOfPositions >= std::numeric_limits<uint16>::max())
//            {
//                str.write(reinterpret_cast<const char*>(unsigneds.data()), unsigneds.size_bytes());
//                ASSERT((flags & areIndices16Bit) == 0);
//            }
//            else
//            {
//                const std::vector<uint16> shorts = { unsigneds.data(), unsigneds.data() + unsigneds.size() };
//                str.write(reinterpret_cast<const char*>(shorts.data()), shorts.size() * sizeof(uint16));
//                ASSERT(flags & areIndices16Bit);
//            }
//        }
//    }
//    if (normals.has_value()) str.write(reinterpret_cast<const char*>(normals->data()), normals->size_bytes());
//    if (uvs.has_value()) str.write(reinterpret_cast<const char*>(uvs->data()), uvs->size_bytes());
//    if (colors.has_value()) str.write(reinterpret_cast<const char*>(colors->data()), colors->size_bytes());
//
//    return true;
//}
//
//Engine::MetaType Engine::StaticMesh::Reflect()
//{
//    MetaType type = MetaType{ MetaType::T<StaticMesh>{}, "StaticMesh", MetaType::Base<Asset>{}, MetaType::Ctor<AssetLoadInfo&>{}, MetaType::Ctor<std::string_view>{} };
//    ReflectAssetType<StaticMesh>(type);
//    return type;
//}