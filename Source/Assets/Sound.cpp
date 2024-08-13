#include "Precomp.h"
#include "Assets/Sound.h"

#include <fmod/fmod.hpp>

#include "Assets/Core/AssetLoadInfo.h"
#include "Core/Audio.h"
#include "Utilities/StringFunctions.h"
#include "Utilities/Reflect/ReflectAssetType.h"

CE::Sound::Sound(std::string_view name) :
	Asset(name, MakeTypeId<Sound>())
{
}

CE::Sound::Sound(AssetLoadInfo& loadInfo) :
	Asset(loadInfo)
{
    static constexpr FMOD_MODE mode = FMOD_OPENMEMORY | FMOD_LOOP_NORMAL;

    const std::string soundInMemory = StringFunctions::StreamToString(loadInfo.GetStream());

    FMOD_CREATESOUNDEXINFO info{};
    info.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
    info.length = static_cast<int>(soundInMemory.size());

    const FMOD_RESULT result = Audio::Get().GetCoreSystem().createSound(soundInMemory.data(), mode, &info, &mSound);

    if (result != FMOD_OK)
    {
        LOG(LogAudio, Error, "Sound {} could not be loaded, FMOD error {}", GetName(), static_cast<int>(result));
        return;
    }
}

CE::Sound::~Sound()
{
    if (mSound == nullptr)
    {
        return;
    }

    const FMOD_RESULT result = mSound->release();

    if (result != FMOD_OK)
    {
        LOG(LogAudio, Error, "Sound {} could not be released, FMOD error {}", GetName(), static_cast<int>(result));
    }
}

FMOD::Channel* CE::Sound::Play(Audio::Group group) const
{
    if (mSound == nullptr)
    {
        return nullptr;
    }

    FMOD::Channel* channel{};

    const FMOD_RESULT result = Audio::Get().GetCoreSystem().playSound(mSound, &Audio::Get().GetChannelGroup(group), false, &channel);
    if (result != FMOD_OK)
    {
        LOG(LogAudio, Error, "Sound {} could not be played, FMOD error {}", GetName(), static_cast<int>(result));
    }
    return channel;
}

CE::MetaType CE::Sound::Reflect()
{
    MetaType type = MetaType{ MetaType::T<Sound>{}, "Sound", MetaType::Base<Asset>{}, MetaType::Ctor<AssetLoadInfo&>{}, MetaType::Ctor<std::string_view>{} };
    ReflectAssetType<Sound>(type);
    return type;
}
