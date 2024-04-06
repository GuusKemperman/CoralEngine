#include "Precomp.h"
#include "Utilities/Reflect/ReflectComponentType.h"

#include "Meta/MetaProps.h"
#include "Scripting/ScriptTools.h"

namespace
{
	std::string GetGetComponentFuncName(std::string_view componentTypeName)
	{
		return CE::Format("Get {}", componentTypeName);
	}

	std::string GetRemoveComponentFuncName(std::string_view componentTypeName)
	{
		return CE::Format("Remove {}", componentTypeName);
	}

	std::string GetHasComponentFuncName(std::string_view componentTypeName)
	{
		return CE::Format("Has {}", componentTypeName);
	}
}

void CE::Internal::ReflectComponentType(MetaType& type, bool isEmpty)
{
	type.GetProperties().Add(Props::sComponentTag);

	if (!CanTypeBeReferencedInScripts(type))
	{
		return;
	}

	MetaType& entityType = MetaManager::Get().GetType<entt::entity>();

	if (entityType.TryGetFunc(Internal::GetAddComponentFuncName(type.GetName())) == nullptr)
	{
		ASSERT(WasTypeCreatedByScript(type) && "Call the templated function instead of this one");

		entityType.AddFunc(
			[&type](MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer) -> FuncResult
			{
				World* world = World::TryGetWorldAtTopOfStack();
				ASSERT(world != nullptr && "Reached a scripting context without pushing a world");

				entt::entity entity = *static_cast<entt::entity*>(args[0].GetData());

				Registry& reg = world->GetRegistry();

				const MetaAny existingComponent = reg.TryGet(type.GetTypeId(), entity);

				if (existingComponent != nullptr)
				{
					return Format("Could not add {} to entity {} - this entity already has a component of this type",
						type.GetName(), entt::to_integral(entity));
				}

				return world->GetRegistry().AddComponent(type, entity);
			},
			Internal::GetAddComponentFuncName(type.GetName()),
			MetaFunc::Return{ { type.GetTypeId(), TypeForm::Ref } },
			MetaFunc::Params{ { MakeTypeTraits<const entt::entity&>(), "Entity" } }).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);
	}

	if (!isEmpty)
	{
		entityType.AddFunc(
			[typeId = type.GetTypeId()](MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer) -> FuncResult
			{
				World* world = World::TryGetWorldAtTopOfStack();
				ASSERT(world != nullptr && "Reached a scripting context without pushing a world");

				const entt::entity entity = *static_cast<entt::entity*>(args[0].GetData());

				return world->GetRegistry().TryGet(typeId, entity);
			},
			GetGetComponentFuncName(type.GetName()),
			MetaFuncNamedParam{ { type.GetTypeId(), TypeForm::Ptr } }, // Return value
			std::vector<MetaFuncNamedParam>{ { MakeTypeTraits<const entt::entity&>() } }
		).GetProperties().Add(Props::sIsScriptableTag);
	}
	else
	{
		entityType.AddFunc(
			[typeId = type.GetTypeId()](MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer rvoBuffer) -> FuncResult
			{
				World* world = World::TryGetWorldAtTopOfStack();
				ASSERT(world != nullptr && "Reached a scripting context without pushing a world");

				const entt::entity entity = *static_cast<entt::entity*>(args[0].GetData());

				return MetaAny{ world->GetRegistry().HasComponent(typeId, entity), rvoBuffer };
			},
			GetHasComponentFuncName(type.GetName()),
			MetaFuncNamedParam{ MakeTypeTraits<bool>() }, // Return value
			std::vector<MetaFuncNamedParam>{ { MakeTypeTraits<const entt::entity&>() } }
		).GetProperties().Add(Props::sIsScriptableTag);
	}

	entityType.AddFunc(
		[typeId = type.GetTypeId()](MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer) -> FuncResult
		{
			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr && "Reached a scripting context without pushing a world");

			const entt::entity entity = *static_cast<entt::entity*>(args[0].GetData());
			world->GetRegistry().RemoveComponentIfEntityHasIt(typeId, entity);
			return std::nullopt;
		},
		GetRemoveComponentFuncName(type.GetName()),
		MetaFuncNamedParam{ MakeTypeTraits<void>() }, // Return value
		std::vector<MetaFuncNamedParam>{ { MakeTypeTraits<const entt::entity&>() } }
	).GetProperties().Add(Props::sIsScriptableTag);
}

void CE::Internal::UnreflectComponentType(MetaType& type)
{
	MetaType& entityType = MetaManager::Get().GetType<entt::entity>();

	static constexpr TypeTraits entityRef = MakeTypeTraits<const entt::entity&>();

	[[maybe_unused]] size_t numRemoved = entityType.RemoveFunc(Internal::GetAddComponentFuncName(type.GetName()),
		MakeFuncId({ type.GetTypeId(), TypeForm::Ref }, { entityRef }));

	numRemoved += entityType.RemoveFunc(GetGetComponentFuncName(type.GetName()),
		MakeFuncId({ type.GetTypeId(), TypeForm::Ptr }, { entityRef }));

	// Entity will only have one of these functions, not both.
	numRemoved += entityType.RemoveFunc(GetRemoveComponentFuncName(type.GetName()), MakeFuncId<void(const entt::entity&)>());
	numRemoved += entityType.RemoveFunc(GetHasComponentFuncName(type.GetName()), MakeFuncId<bool(const entt::entity&)>());

	if (numRemoved != 3)
	{
		LOG(LogMeta, Error, "Failed to unreflect component {}, expected to remove 3 functions, but removed {} instead",
			type.GetName(),
			numRemoved);
	}
}
