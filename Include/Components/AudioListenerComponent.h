#pragma once
#include "Meta/MetaReflect.h"

namespace CE
{
	class AudioListenerComponent
	{
	public:

		float mVolume = 1.0f;

	private:
		
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AudioListenerComponent);
	};


}