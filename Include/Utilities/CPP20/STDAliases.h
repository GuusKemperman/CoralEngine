#pragma once

#if __cplusplus > 201703L // If we have C++20 or higher

#include <span>
#include <format>
#include <source_location>

#define CONSTEVAL consteval
#define UNLIKELY [[unlikely]]
#define LIKELY [[likely]]

#define STATIC_SPECIALIZATION 

namespace CE
{
	template<typename T, size_t Size = std::dynamic_extent>
	using Span = std::span<T, Size>;

	template <class... T>
	std::string Format(const std::format_string<T...> fmt, T&&... args)
	{
		return std::vformat(fmt.get(), std::make_format_args(args...));
	}
}

#else // If we don't have C++20

#include "Span.h"

#ifdef _MSC_VER 

#pragma warning(push)
#pragma warning(disable : 4702) // Unreachable code

#endif // _MSC_VER

#define FMT_HEADER_ONLY
#include "fmt/format.h"

#ifdef _MSC_VER

#pragma warning(pop)

#endif // _MSC_VER

#define CONSTEVAL constexpr
#define UNLIKELY
#define LIKELY

#ifdef __clang__
#define STATIC_SPECIALIZATION
#else
// MSVC has a bug and does not conform to the standard.
#define STATIC_SPECIALIZATION static
#endif
namespace CE
{
	template<typename T, size_t Size = tcb::dynamic_extent>
	using Span = tcb::span<T, Size>;

	template <class... T>
	std::string Format(const fmt::format_string<T...> fmt, T&&... args)
	{
		return fmt::vformat(fmt.get(), fmt::make_format_args(args...));
	}
}
#endif

namespace CE
{
#define ENGINE_ALLOCA _alloca
#define FORCE_INLINE __forceinline
}
