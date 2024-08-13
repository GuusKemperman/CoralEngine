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

void CE::Internal::ReflectRuntimeComponentType(MetaType& type, bool isEmpty)
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
				World& world = *static_cast<World*>(args[0].GetData());
				const entt::entity entity = *static_cast<entt::entity*>(args[1].GetData());

				Registry& reg = world.GetRegistry();

				const MetaAny existingComponent = reg.TryGet(type.GetTypeId(), entity);

				if (existingComponent != nullptr)
				{
					return Format("Could not add {} to entity {} - this entity already has a component of this type",
						type.GetName(), entt::to_integral(entity));
				}

				return world.GetRegistry().AddComponent(type, entity);
			},
			Internal::GetAddComponentFuncName(type.GetName()),
			MetaFunc::Return{ { type.GetTypeId(), TypeForm::Ref } },
			MetaFunc::Params{ { MakeTypeTraits<World&>() }, { MakeTypeTraits<entt::entity>() }}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);
	}

	if (!isEmpty)
	{
		entityType.AddFunc(
			[typeId = type.GetTypeId()](MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer) -> FuncResult
			{
				World& world = *static_cast<World*>(args[0].GetData());
				const entt::entity entity = *static_cast<entt::entity*>(args[1].GetData());
				return world.GetRegistry().TryGet(typeId, entity);
			},
			GetGetComponentFuncName(type.GetName()),
			MetaFuncNamedParam{ { type.GetTypeId(), TypeForm::Ptr } }, // Return value
			std::vector<MetaFuncNamedParam>{ { MakeTypeTraits<World&>() }, { MakeTypeTraits<entt::entity>() } }
		).GetProperties().Add(Props::sIsScriptableTag);
	}

	entityType.AddFunc(
		[typeId = type.GetTypeId()](MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer rvoBuffer) -> FuncResult
		{
			const World& world = *static_cast<const World*>(args[0].GetData());
			const entt::entity entity = *static_cast<entt::entity*>(args[1].GetData());

			return MetaAny{ world.GetRegistry().HasComponent(typeId, entity), rvoBuffer };
		},
		GetHasComponentFuncName(type.GetName()),
		MetaFuncNamedParam{ MakeTypeTraits<bool>() }, // Return value
		std::vector<MetaFuncNamedParam>{ { MakeTypeTraits<const World&>() }, { MakeTypeTraits<const entt::entity&>() } }
	).GetProperties().Add(Props::sIsScriptableTag);
	

	entityType.AddFunc(
		[typeId = type.GetTypeId()](MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer) -> FuncResult
		{
			World& world = *static_cast<World*>(args[0].GetData());
			const entt::entity entity = *static_cast<entt::entity*>(args[1].GetData());

			world.GetRegistry().RemoveComponentIfEntityHasIt(typeId, entity);
			return std::nullopt;
		},
		GetRemoveComponentFuncName(type.GetName()),
		MetaFuncNamedParam{ MakeTypeTraits<void>() }, // Return value
		std::vector<MetaFuncNamedParam>{ { MakeTypeTraits<World&>() }, { MakeTypeTraits<const entt::entity&>() } }
	).GetProperties().Add(Props::sIsScriptableTag);
}

void CE::Internal::UnreflectComponentType(MetaType& type)
{
	MetaType& entityType = MetaManager::Get().GetType<entt::entity>();

	static constexpr TypeTraits entityRef = MakeTypeTraits<const entt::entity&>();

	[[maybe_unused]] size_t numRemoved = entityType.RemoveFunc(GetAddComponentFuncName(type.GetName()),
		MakeFuncId({ type.GetTypeId(), TypeForm::Ref }, { MakeTypeTraits<World&>(), entityRef }));

	numRemoved += entityType.RemoveFunc(GetGetComponentFuncName(type.GetName()),
		MakeFuncId({ type.GetTypeId(), TypeForm::Ptr }, { MakeTypeTraits<World&>(), entityRef }));

	// Entity will only have one of these functions, not both.
	numRemoved += entityType.RemoveFunc(GetRemoveComponentFuncName(type.GetName()), MakeFuncId<void(World&, const entt::entity&)>());
	numRemoved += entityType.RemoveFunc(GetHasComponentFuncName(type.GetName()), MakeFuncId<bool(const World&, const entt::entity&)>());

	// Empty components (whose size == 1) will not have a Get function
	const bool canBeEmptyType = type.GetSize() == 1;
	if (numRemoved == 4 || (canBeEmptyType && numRemoved == 3))
	{
		LOG(LogMeta, Error, "Failed to unreflect component {}, removed {} functions",
			type.GetName(),
			numRemoved);
	}
}
