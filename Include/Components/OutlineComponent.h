#pragma once
#include "Meta/MetaReflect.h"
#include "BasicDataTypes/Colors/LinearColor.h"

namespace CE
{
	class PostPrOutlineComponent
	{
	public:
		LinearColor mColor{ 1.0f };
		float mThickness = 1.5f;
		float mBias = 0.05f;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(PostPrOutlineComponent);
	};
}

