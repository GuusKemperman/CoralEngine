#pragma once
#include "Meta/MetaReflect.h"
#include "Assets/Core/AssetHandle.h"

namespace CE
{
	class Texture;
	class ToneMappingComponent
	{
	public:
		AssetHandle<Texture> mLUTtexture{};
		glm::vec2 mNumberOfBlocks{};
		bool mInvertLUTOnY{};
		float mExposure = 0.0f;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ToneMappingComponent);
	};
}

