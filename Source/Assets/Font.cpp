#include "Precomp.h"
#include "Assets/Font.h"

#include "Assets/Core/AssetLoadInfo.h"
#include "Utilities/StringFunctions.h"
#include "Utilities/Reflect/ReflectAssetType.h"

CE::Font::Font(std::string_view name) :
    Asset(name, MakeTypeId<Font>())
{
}

CE::Font::Font(AssetLoadInfo& loadInfo) :
    Asset(loadInfo),
    mFileContents(StringFunctions::StreamToString(loadInfo.GetStream()))
{
}


CE::MetaType CE::Font::Reflect()
{
    MetaType type = MetaType{ MetaType::T<Font>{}, "Font", MetaType::Base<Asset>{}, MetaType::Ctor<AssetLoadInfo&>{}, MetaType::Ctor<std::string_view>{} };
    type.GetProperties().Add(Props::sCannotReferenceOtherAssetsTag);
    ReflectAssetType<Font>(type);
    return type;
}
