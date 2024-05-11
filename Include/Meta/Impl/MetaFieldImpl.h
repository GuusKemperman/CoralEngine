#pragma once
#include "Meta/Fwd/MetaFieldFwd.h"

#include "Meta/MetaManager.h"
#include "Meta/MetaTypeId.h"

template<typename T, typename Outer>
CE::MetaField::MetaField(const MetaType& outer, T Outer::* ptr, const std::string_view name) :
	MetaField(outer, MetaManager::Get().GetType<T>(),
		// UB since we are dereferencing a nullptr, but there is currently no legal
		// way of doing this in c++17 or c++20.
		static_cast<uint32>(reinterpret_cast<uintptr>(&reinterpret_cast<const volatile char&>(reinterpret_cast<Outer*>(size_t{ 0 })->*ptr))),
		name)
{
#ifdef ASSERTS_ENABLED
	AssertThatOuterMatches(MakeTypeId<Outer>());
#endif // ASSERTS_ENABLED
}