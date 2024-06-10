#pragma once
#include "Assets/Core/AssetHandle.h"
#include "Meta/MetaReflect.h"
#include "Rendering/SkinnedMeshDefines.h"

namespace CE
{
	class SkinnedMesh;
	class Animation;
	class Material;

	class SkinnedMeshComponent
	{
	public:
		AssetHandle<SkinnedMesh> mSkinnedMesh{};

		AssetHandle<Animation> mAnimation{};
		AssetHandle<Animation> mPreviousAnimation{};

		AssetHandle<Material> mMaterial{};

		float mCurrentTime = 0.0f;
		float mPrevAnimTime = 0.0f;
		float mAnimationSpeed = 1.0f;
		// Blend from previous animation 
		// Previous 0.0f to current 1.0f
		float mBlendWeight = 1.0f;
		// Time it takes to blend animations in seconds
		float mBlendTime = 0.2f;

		bool mHighlightedMesh = false;

		std::array<glm::mat4x4, MAX_BONES> mFinalBoneMatrices = []
			{
				std::array<glm::mat4x4, MAX_BONES> bones{};
				std::fill(bones.begin(), bones.end(), glm::mat4x4{1.0f});
				return bones;
			}();

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(SkinnedMeshComponent);
	};
}