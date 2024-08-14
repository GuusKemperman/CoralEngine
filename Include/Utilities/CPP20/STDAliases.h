#pragma once

#ifdef _MSC_VER 
#pragma warning(push)
#pragma warning(disable : 4702) // Unreachable code
#endif // _MSC_VER

#define FMT_HEADER_ONLY
#include "fmt/format.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

namespace CE
{
	template <class... T>
	std::string Format(const fmt::format_string<T...> fmt, T&&... args)
	{
		return fmt::vformat(fmt.get(), fmt::make_format_args(args...));
	}
}

// Older versions of MSVC did not implement CONSTEVAL correctly,
// see compiler explorer: https://godbolt.org/z/97rsn91eh
// By using this macro, older versions will still compile,
// they'll just do more work at runtime.
#if defined(_MSC_FULL_VER) && _MSC_FULL_VER < 194133923
#define CONSTEVAL constexpr
#else
#define CONSTEVAL consteval
#endif

#define UNLIKELY [[unlikely]]
#define LIKELY [[likely]]
#define ENGINE_ALLOCA _alloca
#define FORCE_INLINE __forceinline
