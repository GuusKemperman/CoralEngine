#include "Precomp.h"
#include "Assets/Animation/Animation.h"
#include "Assets/SkinnedMesh.h"
#include "Assets/Animation/Bone.h"

#include "Meta/MetaType.h"
#include "Utilities/StringFunctions.h"
#include "Utilities/Reflect/ReflectAssetType.h"
#include "Utilities/ClassVersion.h"
#include "Assets/Core/AssetLoadInfo.h"
#include "Assets/Core/AssetSaveInfo.h"

namespace cereal
{
	class BinaryOutputArchive;
	class BinaryInputArchive;

	inline void save(BinaryOutputArchive& ar, const Engine::KeyPosition& position)
	{
		ar(position.position, position.timeStamp);
	}

	inline void load(BinaryInputArchive& ar, Engine::KeyPosition& position)
	{
		ar(position.position, position.timeStamp);
	}

	inline void save(BinaryOutputArchive& ar, const Engine::KeyRotation& rotation)
	{
		ar(rotation.orientation, rotation.timeStamp);
	}

	inline void load(BinaryInputArchive& ar, Engine::KeyRotation& rotation)
	{
		ar(rotation.orientation, rotation.timeStamp);
	}

	inline void save(BinaryOutputArchive& ar, const Engine::KeyScale& scale)
	{
		ar(scale.scale, scale.timeStamp);	
	}

	inline void load(BinaryInputArchive& ar, Engine::KeyScale& scale)
	{
		ar(scale.scale, scale.timeStamp);
	}
}

Engine::Animation::Animation(const std::string_view name)
	: Asset(name, MakeTypeId<Animation>())
{
}

void LoadHierarchyFromGSON(const Engine::BinaryGSONObject& obj, Engine::AnimNode& dest)
{
	obj.GetGSONMember("Transform") >> dest.mTransform;
	obj.GetGSONMember("Name") >> dest.mName;
	
	const std::vector<Engine::BinaryGSONObject>& children = obj.GetChildren();

	for (Engine::BinaryGSONObject child : children)
	{
		Engine::AnimNode newNode;
		LoadHierarchyFromGSON(child, newNode);
		dest.mChildren.push_back(newNode);
	}
}

Engine::Animation::Animation(AssetLoadInfo& loadInfo) :
	Asset(loadInfo)
{
	BinaryGSONObject obj{};
	const bool success = obj.LoadFromBinary(loadInfo.GetStream());

	if (!success)
	{
		LOG(LogAssets, Error, "Could not load animation {}, GSON parsing failed", GetName());
		return;
	}

	const BinaryGSONMember* serializedDuration = obj.TryGetGSONMember("Duration");
	const BinaryGSONMember* serializedTicksPerSecond = obj.TryGetGSONMember("TicksPerSecond");
	const BinaryGSONObject* serializedBones = obj.TryGetGSONObject("Bones");
	const BinaryGSONObject* serializedHierarchy = obj.TryGetGSONObject("Hierarchy");

	if (serializedDuration == nullptr
		|| serializedTicksPerSecond == nullptr
		|| serializedBones == nullptr
		|| serializedHierarchy == nullptr)
	{
		LOG(LogAssets, Error, "Could not load animation {}, there are missing values", GetName());
		return;
	}

	*serializedDuration >> mDuration;
	*serializedTicksPerSecond >> mTickPerSecond;

	if (serializedHierarchy->GetChildren().empty())
	{
		LOG(LogAssets, Error, "Could not load animation {}, empty hierarchy", GetName());
		return;
	}

	if (serializedBones->GetChildren().empty())
	{
		LOG(LogAssets, Error, "Could not load animation {}, no bones found", GetName());
		return;
	}

	auto& bones = serializedBones->GetChildren();

	mBones = std::vector<Bone>();

	for (BinaryGSONObject bone : bones)
	{
		AnimData data{};
		std::string name;
		name = bone.GetName();

		bone.GetGSONMember("KeyPositions") >> data.mPositions;
		bone.GetGSONMember("KeyRotations") >> data.mRotations;
		bone.GetGSONMember("KeyScalings") >> data.mScales;

		mBones.push_back(Bone(name, data));
	}

	LoadHierarchyFromGSON(serializedHierarchy->GetChildren().at(0), mRootNode);
}

void SaveHierarchyToGSON(const Engine::AnimNode& node, Engine::BinaryGSONObject& obj)
{
	auto& currentNode = obj.AddGSONObject(node.mName);

	currentNode.AddGSONMember("Transform") << node.mTransform;
	currentNode.AddGSONMember("Name") << node.mName;

	for (Engine::AnimNode child : node.mChildren)
	{
		SaveHierarchyToGSON(child, currentNode);
	}
}

void Engine::Animation::OnSave(AssetSaveInfo& saveInfo) const
{
	if (mBones.empty())
	{
		LOG(LogAssets, Error, "Unable to save animation {}, No bones found", this->GetName());
	}

	BinaryGSONObject obj{ GetName() };

	obj.AddGSONMember("Duration") << mDuration;
	obj.AddGSONMember("TicksPerSecond") << mTickPerSecond;
	
	auto& objectBones = obj.AddGSONObject("Bones");

	for (Bone bone : mBones)
	{
		auto& currentBone = objectBones.AddGSONObject(bone.mName);

		currentBone.AddGSONMember("KeyPositions") << bone.mPositions;
		currentBone.AddGSONMember("KeyRotations") << bone.mRotations;
		currentBone.AddGSONMember("KeyScalings") << bone.mScales;
	}

	auto& hierarchy = obj.AddGSONObject("Hierarchy");

	SaveHierarchyToGSON(mRootNode, hierarchy);

	obj.SaveToBinary(saveInfo.GetStream());
}

Engine::MetaType Engine::Animation::Reflect()
{
	MetaType type = MetaType{ MetaType::T<Animation>{}, "Animation", MetaType::Base<Asset>{}, MetaType::Ctor<AssetLoadInfo&>{}, MetaType::Ctor<std::string_view>{} };

	ReflectAssetType<Animation>(type);
	return type;
}

const Engine::Bone* Engine::Animation::FindBone(const std::string_view name) const
{
	auto iter = std::find_if(mBones.begin(), mBones.end(),
		[&](const Engine::Bone& bone)
		{
			return bone.mName == name; 
		}
	);
	if (iter == mBones.end())
	{
		return nullptr;
	}
	else
	{
		return &(*iter);
	}
}
