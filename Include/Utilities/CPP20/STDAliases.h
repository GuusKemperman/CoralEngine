#pragma once

#if __cplusplus > 201703L // If we have C++20 or higher

#include <span>
#include <format>
#include <source_location>

#define CONSTEVAL consteval
#define UNLIKELY [[unlikely]]
#define LIKELY [[likely]]

#define STATIC_SPECIALIZATION 

namespace Engine
{
	template<typename T, size_t Size = std::dynamic_extent>
	using Span = std::span<T, Size>;

	using SourceLocation = std::source_location;

	template <class... T>
	std::string Format(const std::format_string<T...> fmt, T&&... args)
	{
		return std::vformat(fmt.get(), std::make_format_args(args...));
	}
}

#else // If we don't have C++20

#include "Span.h"
#include "SourceLocation.h"

#pragma warning(push)
#pragma warning(disable : 4702) // Unreachable code

#define FMT_HEADER_ONLY
#include "fmt/format.h"

#pragma warning(pop)

#define CONSTEVAL constexpr
#define UNLIKELY
#define LIKELY

#ifdef __clang__
#define STATIC_SPECIALIZATION
#else
// MSVC has a bug and does not conform to the standard.
#define STATIC_SPECIALIZATION static
#endif
namespace Engine
{
	template<typename T, size_t Size = tcb::dynamic_extent>
	using Span = tcb::span<T, Size>;

	using SourceLocation = EarlySTD::source_location;

	template <class... T>
	std::string Format(const fmt::format_string<T...> fmt, T&&... args)
	{
		return fmt::vformat(fmt.get(), fmt::make_format_args(args...));
	}
}
#endif

namespace Engine
{
#ifdef PLATFORM_***REMOVED***

#define ENGINE_ALLOCA alloca
#define FORCE_INLINE inline

#elif PLATFORM_WINDOWS

#define ENGINE_ALLOCA _alloca
#define FORCE_INLINE __forceinline

#else 
	static_assert(false, "No platform")
#endif
}
