#pragma once
#include "Meta/MetaManager.h"
#include "Meta/MetaType.h"
#include "Utilities/Reflect/ReflectFieldType.h"

template<typename T>
struct Reflector<std::optional<T>>
{
	static_assert(Engine::sIsReflectable<T>, "Cannot reflect an optional of a type that is not reflected");

	static Engine::MetaType Reflect()
	{
		using namespace Engine;
		const MetaType& basedOnType = MetaManager::Get().GetType<T>();
		MetaType optType{ MetaType::T<std::optional<T>>{}, Format("Optional {}", basedOnType.GetName()) };
		ReflectFieldType<std::optional<T>>(optType);

		return optType;
	}
	static constexpr bool sIsSpecialized = true;
};
