#pragma once

namespace CE
{
	struct RenderCommandQueue;

	struct DebugDraw
	{
		enum Enum
		{
			General = 1 << 0,
			Gameplay = 1 << 1,
			Physics = 1 << 2,
			Sound = 1 << 3,
			Rendering = 1 << 4,
			AINavigation = 1 << 5,
			AIDecision = 1 << 6,
			Editor = 1 << 7,
			AccelStructs = 1 << 8,
			Particles = 1 << 9,
			All = 0xFFFFFFFF
		};
	};

	struct Plane
	{
		enum Enum
		{
			XY,
			XZ,
			YZ
		};
	};

	extern DebugDraw::Enum sDebugDrawFlags;

#ifdef EDITOR // Optimizing away debug lines on non editor builds
#define DEBUG_DRAWING_ENABLED
#endif

#ifdef DEBUG_DRAWING_ENABLED
#define DEBUG_DRAW_FUNC_START 
#define DEBUG_DRAW_FUNC_END ;
#else
#define DEBUG_DRAW_FUNC_START inline
#define DEBUG_DRAW_FUNC_END  {}
#endif

#ifdef DEBUG_DRAWING_ENABLED
	bool IsDebugDrawCategoryVisible(DebugDraw::Enum category);
#else
	inline bool IsDebugDrawCategoryVisible([[maybe_unused]] DebugDraw::Enum category) { return false; }
#endif // DEBUG_DRAWING_ENABLED

	DEBUG_DRAW_FUNC_START void AddDebugLine([[maybe_unused]] RenderCommandQueue& commandQueue,
		[[maybe_unused]] DebugDraw::Enum category,
		[[maybe_unused]] glm::vec3 from,
		[[maybe_unused]] glm::vec3 to,
		[[maybe_unused]] glm::vec4 color) DEBUG_DRAW_FUNC_END;

	DEBUG_DRAW_FUNC_START void AddDebugCircle([[maybe_unused]] RenderCommandQueue& commandQueue,
		[[maybe_unused]] DebugDraw::Enum category,
		[[maybe_unused]] glm::vec3 center,
		[[maybe_unused]] float radius,
		[[maybe_unused]] glm::vec4 color) DEBUG_DRAW_FUNC_END;

	DEBUG_DRAW_FUNC_START void AddDebugSphere([[maybe_unused]] RenderCommandQueue& commandQueue,
		[[maybe_unused]] DebugDraw::Enum category,
		[[maybe_unused]] glm::vec3 center,
		[[maybe_unused]] float radius,
		[[maybe_unused]] glm::vec4 color) DEBUG_DRAW_FUNC_END;

	DEBUG_DRAW_FUNC_START void AddDebugSquare([[maybe_unused]] RenderCommandQueue& commandQueue,
		[[maybe_unused]] DebugDraw::Enum category,
		[[maybe_unused]] glm::vec3 center,
		[[maybe_unused]] float size,
		[[maybe_unused]] glm::vec4 color) DEBUG_DRAW_FUNC_END;

	DEBUG_DRAW_FUNC_START void AddDebugBox([[maybe_unused]] RenderCommandQueue& commandQueue,
		[[maybe_unused]] DebugDraw::Enum category,
		[[maybe_unused]] glm::vec3 center,
		[[maybe_unused]] glm::vec3 halfExtents,
		[[maybe_unused]] glm::vec4 color) DEBUG_DRAW_FUNC_END;

	DEBUG_DRAW_FUNC_START void AddDebugCylinder([[maybe_unused]] RenderCommandQueue& commandQueue,
		[[maybe_unused]] DebugDraw::Enum category, 
		[[maybe_unused]] glm::vec3 from, 
		[[maybe_unused]] glm::vec3 to, 
		[[maybe_unused]] float radius, 
		[[maybe_unused]] glm::vec4 color) DEBUG_DRAW_FUNC_END;
}