#pragma once

#if defined(PLATFORM_***REMOVED***) && defined(EDITOR)
static_assert(false, "EngineDebug or EngineRelease configuration is not supported for ***REMOVED***");
#endif

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

#ifdef __clang__

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#endif

#include "entt/entity/component.hpp"
#include "entt/entity/entity.hpp"
#include "entt/entity/registry.hpp"
#include "entt/entity/sparse_set.hpp"
#include "entt/entity/storage.hpp"
#include "entt/entity/view.hpp"

#ifdef __clang__
#pragma clang diagnostic pop
#endif // __clang__

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4201)
#endif

#define GLM_FORCE_LEFT_HANDED
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp> 
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/type_ptr.hpp>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef PLATFORM_WINDOWS
#define DX12
#endif // PLATFORM_WINDOWS

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

#ifdef EDITOR
#define IM_ASSERT(exp) ASSERT(exp)

#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_stdlib.h"
#include "imgui/IconsFontAwesome.h"

#endif // EDITOR

template<bool flag = false> void static_no_match() { static_assert(flag, "No match, see surrounding code for the possible cause."); }

template <class> constexpr bool AlwaysFalse = false;

#include "Utilities/CommonMetaProperties.h"
#include "Utilities/EnumString.h"

