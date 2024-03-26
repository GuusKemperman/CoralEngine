#pragma once
#include "Utilities/Math.h"

namespace Engine
{
	class TransformComponent;
	class TransformedAABB;

	// Will completely ignore all rotations
	class AABBCollider
	{
	public:
		TransformedAABB CreateTransformedAABB(const TransformComponent& transform) const;

		glm::vec2 mHalfExtends{};
	};
	

}
