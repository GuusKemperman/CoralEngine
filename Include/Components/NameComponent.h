#pragma once
#include "Meta/MetaReflect.h"

namespace CE
{
	class Registry;

	class NameComponent
	{
	public:
		std::string mName{};

		static std::string_view GetDisplayName(const Registry& registry, entt::entity entity);

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(NameComponent);
	};
}
