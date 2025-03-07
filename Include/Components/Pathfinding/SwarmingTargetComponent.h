#pragma once
#include "Meta/MetaReflect.h"
#include "Utilities/ASync.h"
#include "Utilities/Geometry2d.h"

namespace CE
{
	class SwarmingTargetComponent
	{
	public:
		float mDesiredRadius = 150.0f;
		int mNumberOfSmoothingSteps = 2;

		struct FlowField
		{
			TransformedAABB GetCellBox(const int x, const int y) const
			{
				const glm::vec2 cellMin = glm::vec2{ static_cast<float>(x), static_cast<float>(y) } *mSpacing + mCellsTopLeftWorldPosition - glm::vec2{ mSpacing * .5f };
				const glm::vec2 cellMax = cellMin + glm::vec2{ mSpacing };
				return { cellMin, cellMax };
			}

			glm::vec2 mCellsTopLeftWorldPosition{};
			int mFlowFieldWidth{};
			float mSpacing{};

			std::vector<glm::vec2> mFlowField{};
		};
		FlowField mCurrent{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(SwarmingTargetComponent);
	};
}
