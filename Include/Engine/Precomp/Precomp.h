#pragma once

#if defined(PLATFORM_***REMOVED***) && defined(EDITOR)
static_assert(false, "EngineDebug or EngineRelease configuration is not supported for ***REMOVED***");
#endif

#include <iostream>
#include <sstream>
#include <vector>
#include <assert.h>
#include <memory>
#include <unordered_map>
#include <filesystem>
#include <optional>
#include <fstream>
#include <functional>
#include <variant>
#include <numeric>

#include "entt/entt.hpp"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4201)
#endif

#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp> 
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/type_ptr.hpp>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef PLATFORM_PC
#define OPEN_GL
#endif // PLATFORM_PC

// Will be removed once we switch
// to directX.
#ifdef OPEN_GL

#include "glad/glad.h"

void _CheckGL(const char* f, int l);
void _CheckFrameBuffer(const char* f, int l);

#define CheckGL() { _CheckGL( __FILE__, __LINE__ ); }
#define CheckFrameBuffer() { _CheckFrameBuffer(__FILE__, __LINE__); }
#else
#define CheckGL()
#define CheckFrameBuffer()
#endif // OPEN_GL

namespace Engine
{
	using EntityType = std::underlying_type_t<entt::entity>;
}

#include "BasicDataTypes/ArithmeticAliases.h"
#include "Utilities/CPP20/STDAliases.h"

#include "Core/Logger.h"
#include "BasicDataTypes/Name.h"

#if defined(_DEBUG) || defined(EDITOR)
#define ASSERTS_ENABLED
#endif

#ifdef ASSERTS_ENABLED

#define ASSERT_LOG(condition, format, ...)\
if (condition) {}\
else { UNLIKELY; LOG(LogTemp, Fatal, "Assert failed: " #condition " - " format, __VA_ARGS__); }

#define ASSERT_LOG_TRIVIAL(condition, string)\
if (condition) {}\
else { UNLIKELY; LOG_TRIVIAL(LogTemp, Fatal, "Assert failed: " #string); }

#else
#define ASSERT_LOG(...)
#define ASSERT_LOG_TRIVIAL(...)
#endif // ASSERTS_ENABLED

#define ASSERT(condition) ASSERT_LOG_TRIVIAL(condition, "")
#define ABORT LOG_TRIVIAL(LogTemp, Fatal, "Aborted");

#ifdef EDITOR
#define IM_ASSERT(exp) ASSERT(exp)

#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_stdlib.h"
#endif // EDITOR

template<typename T, typename = void>
constexpr bool IsFullyDefined = false;

template<typename T>
constexpr bool IsFullyDefined<T, decltype(typeid(T), void())> = true;

template<bool flag = false> void static_no_match() { static_assert(flag, "No match, see surrounding code for the possible cause."); }

template <class> constexpr bool AlwaysFalse = false;

#include "Utilities/Math.h"
#include "Core/CommonMetaProperties.h"

#include "Utilities/EnumString.h"

