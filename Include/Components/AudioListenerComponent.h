#pragma once
#include "Meta/MetaReflect.h"

namespace CE
{
	class World;

	class AudioListenerComponent
	{
	public:
		static entt::entity GetSelected(const World& world);
		static bool IsSelected(const World& world, entt::entity cameraOwner);

		static void Select(World& world, entt::entity cameraOwner);
		static void Deselect(World& world);

		float mVolume = 1.0f;
		float mPitch = 1.0f;
		
		bool mUseLowPass = false;

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