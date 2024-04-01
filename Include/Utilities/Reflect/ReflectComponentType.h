#pragma once
#include "World/World.h"
#include "World/Registry.h"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaProps.h"
#include "Meta/MetaAny.h"

namespace CE
{
	namespace Internal
	{
		inline std::string GetAddComponentFuncName(std::string_view componentTypeName)
		{
			return Format("Add {}", componentTypeName);
		}

		void ReflectComponentType(MetaType& type, bool isEmpty);

		// Removes the functions added during ReflectComponentType
		void UnreflectComponentType(MetaType& type);
	}

	/*
	Adds the functions needed to Add, Get or Remove a component through scripts.
	*/
	template<typename T>
	void ReflectComponentType(MetaType& type)
	{
		MetaType& entityType = MetaManager::Get().GetType<entt::entity>();

		static constexpr bool isEmpty = std::is_empty_v<T>;

		MetaFunc& addComponentFunc = entityType.AddFunc(
			[](MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer) -> FuncResult
			{
				World* world = World::TryGetWorldAtTopOfStack();
				ASSERT(world != nullptr && "Reached a scripting context without pushing a world");

				entt::entity entity = *static_cast<entt::entity*>(args[0].GetData());

				Registry& reg = world->GetRegistry();

				if (reg.HasComponent<T>(entity))
				{
					return Format("Could not add {} to entity {} - this entity already has a component of this type",
						MakeTypeName<T>(), entt::to_integral(entity));
				}

				if constexpr (!isEmpty)
				{
					return MetaAny{ MakeTypeInfo<T>(), &reg.AddComponent<T>(entity) };
				}
				else
				{
					reg.AddComponent<T>(entity);
					return {};
				}
			},
			Internal::GetAddComponentFuncName(type.GetName()),
			MetaFunc::Return{ isEmpty ? MakeTypeTraits<void>() : MakeTypeTraits<T&>() },
			MetaFunc::Params{ { MakeTypeTraits<const entt::entity&>(), "Entity" } });

		if (type.GetProperties().Has(Props::sIsScriptableTag))
		{
			addComponentFunc.GetProperties().Add(Props::sIsScriptableTag);
		}

		Internal::ReflectComponentType(type, isEmpty);
	}
}
