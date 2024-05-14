#pragma once
#include "Meta/MetaReflect.h"

namespace CE
{
	struct BoneInfo;
	class World;

	class AttachToBoneComponent
	{
	public:
		void OnConstruct(World&, entt::entity owner);

		entt::entity mOwner{};

		std::shared_ptr<BoneInfo> mConnectedBone{}; 

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AttachToBoneComponent);
	};
}