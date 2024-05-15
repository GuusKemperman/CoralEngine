#pragma once
#include "Assets/Asset.h"
#include "Assets/Animation/BoneInfo.h"

namespace CE
{
	class Bone;

	struct AnimNode
	{
		std::string mName{};
		glm::mat4x4 mTransform{};
		std::vector<AnimNode> mChildren{};

		// Pointers are stable,
		// Animation::mBones does
		// not need to reallocate.
		const Bone* mBone{};
	};

	class Animation final 
		: public Asset
	{
	public:
		Animation(const std::string_view name);
		Animation(AssetLoadInfo& loadInfo);

		Animation(Animation&&) noexcept = delete;
		Animation(const Animation&) = delete;

		Animation& operator=(Animation&&) = delete;
		Animation& operator=(const Animation&) = delete;

		float mDuration = 0.0;
		float mTickPerSecond = 0.0;
		AnimNode mRootNode{};
		std::vector<Bone> mBones{};

	private:
		friend class AnimationImporter;

		void OnSave(AssetSaveInfo& saveInfo) const override;

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(Animation);
	};
}

#include <cereal/types/vector.hpp>
#include "cereal/archives/binary.hpp"

namespace cereal
{
	class BinaryOutputArchive;
	class BinaryInputArchive;

	inline void save(BinaryOutputArchive& ar, const CE::AnimNode& node)
	{
		ar.saveBinary(&node, sizeof(CE::AnimNode));
	}

	inline void load(BinaryInputArchive& ar, CE::AnimNode& node)
	{
		ar.loadBinary(&node, sizeof(CE::AnimNode));
	}
}