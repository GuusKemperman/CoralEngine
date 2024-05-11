#pragma once
#include "Meta/MetaReflect.h"
#include "BasicDataTypes/Colors/LinearColor.h"

namespace CE
{
	class FogComponent
	{
	public:
		LinearColor mColor{ 1.0f };
		float mNearPlane = 0.0f;
		float mFarPlane = 500.0f;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(FogComponent);
	};
}

