#pragma once
#include "MetaTypeIdFwd.h"

template<typename>
struct Reflector
{
	static constexpr bool sIsSpecialized = false;
};

namespace CE
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
		static constexpr bool HasReflectFunc();

		template <typename T>
		static auto GetReflectFunc()
		{
			return &T::Reflect;
		}
	};

	template <typename T>
	constexpr bool ReflectAccess::HasReflectFunc()
	{
		return std::is_same<decltype(test<T>(0)), std::true_type>::value;
	}

	namespace Internal
	{
#ifdef PLATFORM_***REMOVED***
		// ***REMOVED*** does not understand SFINAE well enough.
		// Since these are mostly for more understandable
		// error messages, we assume that if there is no
		// external reflect, there will be an internal reflect.
		// Any conflicts, such as there both being an internal
		// reflect and an external reflect, will only be caught
		// with PC builds.

		template<typename T>
		static constexpr bool sHasInternalReflect = !Reflector<T>::sIsSpecialized;

		template<typename T>
		static constexpr bool sHasExternalReflect = Reflector<T>::sIsSpecialized;
#elif PLATFORM_WINDOWS
		template<typename T>
		static constexpr bool sHasInternalReflect = ReflectAccess::HasReflectFunc<T>();

		template<typename T>
		static constexpr bool sHasExternalReflect = Reflector<T>::sIsSpecialized;
#else
		static_assert(false, "No platform");
#endif

		void RegisterReflectFunc(TypeId typeId, MetaType(*reflectFunc)());

		template<typename T>
		struct ReflectAtStartup
		{
			inline static bool sDummy = []
				{
					static_assert(!sHasExternalReflect<T> || !sHasInternalReflect<T>, "Both an internal and external reflect function.");
					static_assert(sHasExternalReflect<T> || sHasInternalReflect<T>,
						R"(No external or internal reflect function. You need to make sure the Reflect function is included from wherever you are trying to reflect it.
If you are trying to reflect an std::vector<AssetHandle<Material>>, you need to include AssetHandle.h, Material.h and ReflectVector.h.)");

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
#define REFLECT_AT_START_UP_2(dummyVariableName, type) [[maybe_unused]] static inline const bool CONCAT(__sDummyVariableToReflect, dummyVariableName) = CE::Internal::ReflectAtStartup<type>::sDummy;
#define REFLECT_AT_START_UP_1(type) REFLECT_AT_START_UP_2(type, type)

#define EXPAND( x ) x

/*
	Use when your type is inside a namespace or class, and the namespace or class would
	have to be explicitely specified in the current context

Example:
	// Good
	REFLECT_AT_START_UP(glmVec2, vec2)

	// The namespace does not have to be specified now:
	using namespace glm;
	REFLECT_AT_START_UP(vec2)
*/
#define REFLECT_AT_START_UP(...) EXPAND(GET_MACRO(__VA_ARGS__, REFLECT_AT_START_UP_2, REFLECT_AT_START_UP_1)(__VA_ARGS__))


