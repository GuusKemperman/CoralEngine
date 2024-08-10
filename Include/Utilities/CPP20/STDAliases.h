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

#if __cplusplus >= 202002L // If we have C++20 or higher

#define UNLIKELY [[unlikely]]
#define LIKELY [[likely]]

#define STATIC_SPECIALIZATION 

#else // If we don't have C++20

#define UNLIKELY
#define LIKELY

#ifdef __clang__
#define STATIC_SPECIALIZATION
#else
#define STATIC_SPECIALIZATION static
#endif
#endif

namespace CE
{
#define ENGINE_ALLOCA _alloca
#define FORCE_INLINE __forceinline
}
