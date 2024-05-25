#pragma once
#include "Meta/MetaReflect.h"
#include "Utilities/Geometry2d.h"

namespace CE
{
	class SwarmingTargetComponent
	{
	public:
		// The space between cells,
		// and the space used for
		// local avoidance.
		float mSpacing = 2.5f;

		float mDesiredRadius = 150.0f;

		glm::vec2 mCellsTopLeftWorldPosition{};
		int mFlowFieldWidth{};

		std::vector<glm::vec2> mFlowField{};

		TransformedAABB GetCellBox(const int x, const int y) const
		{
			const glm::vec2 cellMin = glm::vec2{ static_cast<float>(x), static_cast<float>(y) } * mSpacing + mCellsTopLeftWorldPosition - glm::vec2{ mSpacing * .5f };
			const glm::vec2 cellMax = cellMin + glm::vec2{ mSpacing };
			return { cellMin, cellMax };
		}

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(SwarmingTargetComponent);
	};
}
