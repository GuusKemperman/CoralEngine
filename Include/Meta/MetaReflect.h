#pragma once
#include "Meta/MetaTypeId.h"

template<typename T>
struct Reflector
{
};

namespace Engine
{
	class MetaType;

	struct ReflectAccess
	{
	private:
		template <class T>
		static auto test(int) -> decltype(T::Reflect(), std::true_type());

		template <class> static std::false_type test(...);

	public:

		template<typename T>
		static constexpr bool HasReflectFunc() { return std::is_same<decltype(test<T>(0)), std::true_type>::value; }

		template<typename T>
		static auto GetReflectFunc()
		{
			return &T::Reflect;
		}
	};

	namespace Internal
	{
		template<typename T>
		struct HasExternalReflectHelper
		{
			template <class TT>
			static auto test(int) -> decltype(Reflector<TT>::Reflect(), std::true_type());

			template <class> static std::false_type test(...);

			static constexpr bool value = std::is_same<decltype(test<T>(0)), std::true_type>::value;
		};

		template<typename T>
		static constexpr bool sHasInternalReflect = ReflectAccess::HasReflectFunc<T>();

		template<typename T>
		static constexpr bool sHasExternalReflect = HasExternalReflectHelper<T>::value;

		template<typename T, std::enable_if_t<sHasInternalReflect<T>, bool> = true>
		MetaType CallReflect()
		{
			return ReflectAccess::GetReflectFunc<T>()();
		}

		template<typename T, std::enable_if_t<sHasExternalReflect<T>, bool> = true>
		MetaType CallReflect()
		{
			return Reflector<T>::Reflect();
		}

		void RegisterReflectFunc(TypeId typeId, MetaType(*reflectFunc)());

		template<typename T>
		struct ReflectAtStartup
		{
			inline static bool sDummy = []
				{
					static_assert(!sHasExternalReflect<T> || !sHasInternalReflect<T>, "Both an internal and external reflect function.");
					static_assert(sHasExternalReflect<T> || sHasInternalReflect<T>,
						R"(No external or internal reflect function. You need to make sure the Reflect function is included from wherever you are trying to reflect it.
If you are trying to reflect an std::vector<std::shared_ptr<const Material>>, you need to include Material.h, ReflectVector.h and ReflectSmartPtr.h.)");

					if constexpr (sHasInternalReflect<T>)
					{
						Internal::RegisterReflectFunc(MakeTypeId<T>(), ReflectAccess::GetReflectFunc<T>());
					}
					else if constexpr (sHasExternalReflect<T>)
					{
						Internal::RegisterReflectFunc(MakeTypeId<T>(), &Reflector<T>::Reflect);
					}
					return true;
				}();
		};
	}

	template<typename T>
	static constexpr bool sIsReflectable = Internal::sHasInternalReflect<T> != Internal::sHasExternalReflect<T>;
}

// Internal helpers, ignore these
#define CONCAT(a, b) CONCAT_INNER(a, b)
#define CONCAT_INNER(a, b) a ## b
#define GET_MACRO(_1, _2, NAME,...) NAME
#define REFLECT_AT_START_UP_2(dummyVariableName, type) [[maybe_unused]] static inline const bool CONCAT(__sDummyVariableToReflect, dummyVariableName) = Engine::Internal::ReflectAtStartup<type>::sDummy;
#define REFLECT_AT_START_UP_1(type) REFLECT_AT_START_UP_2(type, type)

#define EXPAND( x ) x

/*
	Use when your type is inside a namespace or class, and the namespace or class would
	have to be explicitely specified in the current context

Example:
	// Good
	REFLECT_AT_START_UP(glm::, vec2)

	// The namespace does not have to be specified now:
	using namespace glm;
	REFLECT_AT_START_UP(vec2)
*/
#define REFLECT_AT_START_UP(...) EXPAND(GET_MACRO(__VA_ARGS__, REFLECT_AT_START_UP_2, REFLECT_AT_START_UP_1)(__VA_ARGS__))

#include "Meta/ReflectedTypes/ReflectPrimitiveTypes.h"
#include "Meta/ReflectedTypes/ReflectGLM.h"
#include "Meta/ReflectedTypes/STD/ReflectString.h"
#include "Meta/ReflectedTypes/ReflectENTT.h"
#include "Meta/ReflectedTypes/ReflectImGui.h"

