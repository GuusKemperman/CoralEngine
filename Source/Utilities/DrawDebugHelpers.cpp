#include "Precomp.h"
#include "Utilities/DrawDebugHelpers.h"

#include "glm/gtx/rotate_vector.hpp"

#include "Core/Renderer.h"

CE::DebugDraw::Enum CE::sDebugDrawFlags{};

#ifdef DEBUG_DRAWING_ENABLED

bool CE::IsDebugDrawCategoryVisible(DebugDraw::Enum category)
{
	return category & sDebugDrawFlags;
}

void CE::AddDebugLine(RenderCommandQueue& commandQueue, DebugDraw::Enum category, glm::vec3 from, glm::vec3 to, glm::vec4 color)
{
	if (!(sDebugDrawFlags & category))
	{
		return;
	}

	Renderer::Get().AddLine(commandQueue, from, to, color, color);
}

void CE::AddDebugCircle(RenderCommandQueue& commandQueue, DebugDraw::Enum category, glm::vec3 center, float radius, glm::vec4 color)
{
	if (!(sDebugDrawFlags & category))
	{
		return;
	}

	constexpr float dt = glm::two_pi<float>() / 64.0f;
	float t = 0.0f;

	glm::vec3 v0(center.x + radius * cos(t), center.y + radius * sin(t), center.z);
	for (; t < glm::two_pi<float>() - dt; t += dt)
	{
		const glm::vec3 v1(center.x + radius * cos(t + dt), center.y + radius * sin(t + dt), center.z);
		AddDebugLine(commandQueue, category, v0, v1, color);
		v0 = v1;
	}
}

void CE::AddDebugSphere(RenderCommandQueue& commandQueue, DebugDraw::Enum category, glm::vec3 center, float radius, glm::vec4 color)
{
	if (!(sDebugDrawFlags & category))
	{
		return;
	}

	constexpr float dt = glm::two_pi<float>() / 64.0f;
	float t = 0.0f;

	glm::vec3 v0(center.x + radius * cos(t), center.y + radius * sin(t), center.z);
	for (; t < glm::two_pi<float>() - dt; t += dt)
	{
		const glm::vec3 v1(center.x + radius * cos(t + dt), center.y + radius * sin(t + dt), center.z);
		AddDebugLine(commandQueue, category, v0, v1, color);
		v0 = v1;
	}
}

void CE::AddDebugSquare(RenderCommandQueue& commandQueue, DebugDraw::Enum category, glm::vec3 center, float size, glm::vec4 color)
{
	if (!(sDebugDrawFlags & category))
	{
		return;
	}

	const float s = size * 0.5f;
	const glm::vec3 a = center + glm::vec3{ -s, -s, 0.0f };
	const glm::vec3 b = center + glm::vec3{ -s, s, 0.0f };
	const glm::vec3 c = center + glm::vec3{ s, s, 0.0f };
	const glm::vec3 d = center + glm::vec3{ s, -s, 0.0f };

	AddDebugLine(commandQueue, category, a, b, color);
	AddDebugLine(commandQueue, category, b, c, color);
	AddDebugLine(commandQueue, category, c, d, color);
	AddDebugLine(commandQueue, category, d, a, color);
}

void CE::AddDebugBox(RenderCommandQueue& commandQueue, DebugDraw::Enum category, glm::vec3 center, glm::vec3 halfExtents, glm::vec4 color)
{
	if (!(sDebugDrawFlags & category))
	{
		return;
	}

	// Calculate the minimum and maximum corner points of the box
	const glm::vec3 minCorner = center - halfExtents;
	const glm::vec3 maxCorner = center + halfExtents;

	// Generate the lines for the box
	// Front face
	AddDebugLine(commandQueue, category, glm::vec3(minCorner.x, minCorner.y, minCorner.z), glm::vec3(maxCorner.x, minCorner.y, minCorner.z), color);
	AddDebugLine(commandQueue, category, glm::vec3(maxCorner.x, minCorner.y, minCorner.z), glm::vec3(maxCorner.x, maxCorner.y, minCorner.z), color);
	AddDebugLine(commandQueue, category, glm::vec3(maxCorner.x, maxCorner.y, minCorner.z), glm::vec3(minCorner.x, maxCorner.y, minCorner.z), color);
	AddDebugLine(commandQueue, category, glm::vec3(minCorner.x, maxCorner.y, minCorner.z), glm::vec3(minCorner.x, minCorner.y, minCorner.z), color);

	// Back face
	AddDebugLine(commandQueue, category, glm::vec3(minCorner.x, minCorner.y, maxCorner.z), glm::vec3(maxCorner.x, minCorner.y, maxCorner.z), color);
	AddDebugLine(commandQueue, category, glm::vec3(maxCorner.x, minCorner.y, maxCorner.z), glm::vec3(maxCorner.x, maxCorner.y, maxCorner.z), color);
	AddDebugLine(commandQueue, category, glm::vec3(maxCorner.x, maxCorner.y, maxCorner.z), glm::vec3(minCorner.x, maxCorner.y, maxCorner.z), color);
	AddDebugLine(commandQueue, category, glm::vec3(minCorner.x, maxCorner.y, maxCorner.z), glm::vec3(minCorner.x, minCorner.y, maxCorner.z), color);

	// Connecting lines between the front and back faces
	AddDebugLine(commandQueue, category, glm::vec3(minCorner.x, minCorner.y, minCorner.z), glm::vec3(minCorner.x, minCorner.y, maxCorner.z), color);
	AddDebugLine(commandQueue, category, glm::vec3(maxCorner.x, minCorner.y, minCorner.z), glm::vec3(maxCorner.x, minCorner.y, maxCorner.z), color);
	AddDebugLine(commandQueue, category, glm::vec3(maxCorner.x, maxCorner.y, minCorner.z), glm::vec3(maxCorner.x, maxCorner.y, maxCorner.z), color);
	AddDebugLine(commandQueue, category, glm::vec3(minCorner.x, maxCorner.y, minCorner.z), glm::vec3(minCorner.x, maxCorner.y, maxCorner.z), color);
}

void CE::AddDebugCylinder(RenderCommandQueue& commandQueue, DebugDraw::Enum category, glm::vec3 from, glm::vec3 to, float radius, glm::vec4 color)
{
	if (!(sDebugDrawFlags & category))
	{
		return;
	}

	// Should be atleast 4
	static constexpr uint32 segments = 8;

	// Rotate a point around axis to form cylinder segments
	const float angleIncrease = glm::radians(360.0f / segments);
	float angle = angleIncrease;
	glm::vec3 axis = glm::normalize(to - from);

	glm::vec3 perpendicular{};
	glm::vec3 axisAbs = glm::abs(axis);

	// Find best basis vectors
	if (axisAbs.z > axisAbs.x
		&& axisAbs.z > axisAbs.y)
	{
		perpendicular = glm::vec3(1.0f, 0.0f, 0.0f);
	}
	else
	{
		perpendicular = glm::vec3(0.0f, 0.0f, 1.0f);
	}

	perpendicular = glm::normalize(perpendicular - axis * glm::vec3(perpendicular.x * axis.x + perpendicular.y * axis.y + perpendicular.z * axis.z));
	glm::vec3 segment = glm::rotate(perpendicular, 0.0f, axis) * radius;

	glm::vec3 p1 = segment + from;
	glm::vec3 p3 = segment + to;

	for (uint32 i = 0; i < segments; ++i)
	{
		segment = glm::rotate(perpendicular, angle, axis) * radius;
		glm::vec3 p2 = segment + from;
		glm::vec3 p4 = segment + to;

		AddDebugLine(commandQueue, category, p2, p4, color);
		AddDebugLine(commandQueue, category, p1, p2, color);
		AddDebugLine(commandQueue, category, p3, p4, color);

		p1 = p2;
		p3 = p4;
		angle += angleIncrease;
	}
}

#endif // DEBUG_DRAWING_ENABLED

