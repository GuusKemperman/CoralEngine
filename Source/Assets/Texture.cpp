#include "Precomp.h"
#include "Assets/Texture.h"

#include "Utilities/StringFunctions.h"
#include "Utilities/Reflect/ReflectAssetType.h"
#include "Utilities/StringFunctions.h"
#include "Meta/MetaManager.h"

CE::Texture::Texture(std::string_view name) :
    Asset(name, MakeTypeId<Texture>())
{}

CE::MetaType CE::Texture::Reflect()
{
    MetaType type = MetaType{ MetaType::T<Texture>{}, "Texture", MetaType::Base<Asset>{}, MetaType::Ctor<AssetLoadInfo&>{}, MetaType::Ctor<std::string_view>{} };
    ReflectAssetType<Texture>(type);
    return type;
}