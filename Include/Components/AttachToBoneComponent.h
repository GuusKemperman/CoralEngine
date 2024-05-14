#pragma once
#include "Meta/MetaReflect.h"

namespace CE
{
	class World;

	class AttachToBoneComponent
	{
	public:
		std::string mBoneName{};

	private:
		static void OnInspect(World& world, const std::vector<entt::entity>& entities);

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AttachToBoneComponent);
	};
}