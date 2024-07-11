#pragma once

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

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 5054)
#endif

#include "cereal/cereal.hpp"
#include "cereal/types/pair.hpp"
#include "cereal/types/glm.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/types/unordered_map.hpp"
#include "cereal/types/optional.hpp"
#include "cereal/types/string.hpp"
#include "cereal/archives/binary.hpp"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef PLATFORM_WINDOWS
#define DX12
#endif // PLATFORM_WINDOWS

// Coral Engine
namespace CE {}

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

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4201)
#pragma warning(disable : 4201)
#endif
#include "imgui/auto.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // EDITOR

template<bool flag = false> void static_no_match() { static_assert(flag, "No match, see surrounding code for the possible cause."); }

template <class> constexpr bool AlwaysFalse = false;

#include "Utilities/CommonMetaProperties.h"
#include "Utilities/EnumString.h"
