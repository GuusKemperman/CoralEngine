#pragma once
#include "Assets/Asset.h"
#include "Assets/Animation/BoneInfo.h"

namespace Engine
{
	class Bone;

	struct AnimNode
	{
		std::string mName;
		glm::mat4x4 mTransform;
		std::vector<AnimNode> mChildren;
	};

	class Animation final 
		: public Asset
	{
	public:
		Animation(const std::string_view name);
		Animation(AssetLoadInfo& loadInfo);

		const Bone* FindBone(std::string_view name) const;

		float mDuration = 0.0;
		float mTickPerSecond = 0.0;
		AnimNode mRootNode;
		std::vector<Bone> mBones;
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

	inline void save(BinaryOutputArchive& ar, const Engine::AnimNode& node)
	{
		ar.saveBinary(&node, sizeof(Engine::AnimNode));
	}

	inline void load(BinaryInputArchive& ar, Engine::AnimNode& node)
	{
		ar.loadBinary(&node, sizeof(Engine::AnimNode));
	}
}