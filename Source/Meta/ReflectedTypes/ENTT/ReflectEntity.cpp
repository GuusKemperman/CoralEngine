#include "Precomp.h"

#include "Utilities/ComponentFilter.h"
#include "Meta/MetaReflect.h"
#include "Meta/ReflectedTypes/ReflectENTT.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectFieldType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"
#include "World/World.h"
#include "World/Registry.h"

using namespace CE;
using namespace entt;
using T = entity;

bool sDummy = CE::Internal::ReflectAtStartup<std::vector<T>>::sDummy;

MetaType Reflector<T>::Reflect()
{
	MetaType type{ MetaType::T<T>{}, "Entity" };

	MetaFunc* defaultConstructor = const_cast<MetaFunc*>(type.TryGetDefaultConstructor());
	ASSERT(defaultConstructor != nullptr);

	*defaultConstructor = {
		[](MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer buffers) -> FuncResult
		{
			ASSERT(buffers != nullptr && "The address provided to a constructor may never be nullptr");

			// Unpack needs to know the argument that it
			// needs to forward.
			[[maybe_unused]] size_t argIndex = args.size();

			new (buffers) entt::entity(entt::null);
			return std::nullopt;
		},
		OperatorType::constructor,
		MetaFunc::Return{ MakeTypeTraits<void>() },
		MetaFunc::Params{ }
	};
	ASSERT(type.TryGetDefaultConstructor() != nullptr);

	TypeInfo typeInfo = type.GetTypeInfo();
	typeInfo.mFlags &= ~TypeInfo::IsTriviallyDefaultConstructible;
	type.SetTypeInfo(typeInfo);

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](const entt::entity& entity, nullptr_t) { return entity == entt::null; }, OperatorType::equal).GetProperties();

	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc([](const entt::entity& entity)
		{
			return std::to_string(entt::to_integral(entity));
		}, "ToString").GetProperties().Add(Props::sIsScriptableTag);


	type.AddFunc([](const World& world, const entt::entity& entity)
		{
			return world.GetRegistry().Valid(entity);
		}, "IsAlive").GetProperties().Add(Props::sIsScriptableTag);


	type.AddFunc([](World& world, const entt::entity& entity, const ComponentFilter& component)
		{
			if (component == nullptr)
			{
				return;
			}

			world.GetRegistry().AddComponent(*component.Get(), entity);

		}, "AddComponent").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	ReflectFieldType<T>(type);

	return type;
}
