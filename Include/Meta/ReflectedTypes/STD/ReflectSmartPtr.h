#pragma once
#include "Meta/MetaManager.h"
#include "Meta/MetaProps.h"
#include "Meta/MetaType.h"
#include "Utilities/Reflect/ReflectFieldType.h"

template<typename T>
struct Reflector<std::shared_ptr<const T>>
{
	static_assert(Engine::sIsReflectable<T>, "Cannot reflect a shared_ptr of a type that is not reflected");

	static Engine::MetaType Reflect()
	{
		using namespace Engine;
		const MetaType& basedOnType = MetaManager::Get().GetType<T>();
		MetaType refType{ MetaType::T<std::shared_ptr<const T>>{},
			Format("{} Ref", basedOnType.GetName()) };
		refType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

		refType.AddFunc(
		[](const std::shared_ptr<const T>& value, nullptr_t)
		{
			return value == nullptr;
		}, OperatorType::equal, MetaFunc::ExplicitParams<const std::shared_ptr<const T>&, nullptr_t>{});

		ReflectFieldType<std::shared_ptr<const T>>(refType);

		return refType;
	}
	static constexpr bool sIsSpecialized = true;

};

	// TODO You should be able to call functions of the original type using the smart pointer
	// TODO Also reflect a mutable smart pointer (we currently only needed the const one since we are only using this for assets)
