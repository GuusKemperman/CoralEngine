#pragma once
#include "Meta/MetaReflect.h"
#include "Core/Audio.h"

namespace CE
{
	class World;

	class AudioListenerComponent
	{
	public:
		struct ChannelGroupControl
		{
			Audio::Group mGroup{};
			float mVolume = 1.0f;
			float mPitch = 1.0f;

#ifdef EDITOR
			void DisplayWidget(const std::string& name);
#endif // EDITOR
			
			bool operator==(const ChannelGroupControl& other) const;
			bool operator!=(const ChannelGroupControl& other) const;

			private:
				friend CE::ReflectAccess;
				static CE::MetaType Reflect();
		};

		static entt::entity GetSelected(const World& world);
		static bool IsSelected(const World& world, entt::entity audioListenerOwner);

		static void Select(World& world, entt::entity audioListenerOwner);
		static void Deselect(World& world);

		float mMasterVolume = 1.0f;
		float mMasterPitch = 1.0f;
		
		std::vector<ChannelGroupControl> mChannelGroupControls{};
	private:
		
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AudioListenerComponent);
	};

	class AudioListenerSelectedTag
	{
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AudioListenerSelectedTag);
	};
}

CEREAL_CLASS_VERSION(CE::AudioListenerComponent::ChannelGroupControl, 0);

namespace cereal
{
	template<class Archive>
	void serialize(Archive& ar, CE::AudioListenerComponent::ChannelGroupControl& value, uint32)
	{
		ar(value.mGroup, value.mVolume, value.mPitch);
	}
}

#ifdef EDITOR
IMGUI_AUTO_DEFINE_INLINE(template<>, CE::AudioListenerComponent::ChannelGroupControl, var.DisplayWidget(name);)
#endif // EDITOR