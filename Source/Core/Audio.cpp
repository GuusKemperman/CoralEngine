#include "Precomp.h"
#include "Core/Audio.h"

#include <fmod/fmod.hpp>

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectFieldType.h"
#include "Utilities/StringFunctions.h"

using namespace CE;

Audio::Audio()
{
    // Get the Core System pointer from the Studio System object
    FMOD_RESULT result = FMOD::System_Create(&mCoreSystem);
    if (result != FMOD_OK)
    {
        LOG(LogAudio, Fatal, "FMOD failed to create core audio system, FMOD error {}", static_cast<int>(result));
    }

    result = mCoreSystem->init(512, FMOD_INIT_NORMAL, 0);
    if (result != FMOD_OK)
    {
        LOG(LogAudio, Fatal, "FMOD failed to initialise core audio system, FMOD error {}", static_cast<int>(result));
    }

    for (auto& pair : CE::EnumStringPairsImpl<Audio::Group>().value)
    {
        result = mCoreSystem->createChannelGroup(std::string(pair.second).c_str(), & mChannelGroups[static_cast<int>(pair.first)]);
        if (result != FMOD_OK)
        {
            LOG(LogAudio, Error, "FMOD could not create channel group {}, FMOD error {}", pair.second, static_cast<int>(result));
        }
    }
}

Audio::~Audio()
{
    FMOD_RESULT result{};

    for (FMOD::ChannelGroup* group : mChannelGroups)
    {
        result = group->release();
        if (result != FMOD_OK)
        {
            LOG(LogAudio, Error, "FMOD could not release channel group, FMOD error {}", static_cast<int>(result));
        }
    }
    result = mCoreSystem->release();
    if (result != FMOD_OK)
    {
        LOG(LogAudio, Error, "FMODO could not release core system, FMOD error {}", static_cast<int>(result));
    }
}

void Audio::Update()
{
    FMOD_RESULT result = mCoreSystem->update();
    if (result != FMOD_OK)
    {
        LOG(LogAudio, Error, "FMOD could not update core system, FMOD error {}", static_cast<int>(result));
    }
}

CE::MetaType Reflector<CE::Audio::Group>::Reflect()
{
	using namespace CE;
	using T = Audio::Group;
	MetaType type{ MetaType::T<T>{}, "Group" };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	ReflectFieldType<T>(type);

	return type;
}