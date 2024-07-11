#pragma once

namespace CE
{
	class World;
	struct Line;
	struct TransformedDisk;
	class CharacterComponent;
	class TransformComponent;

	glm::vec2 CombineVelocities(glm::vec2 dominantVelocity, glm::vec2 recessiveVelocity);

	glm::vec2 CalculateAvoidanceVelocity(const World& world,
		entt::entity self,
		float avoidanceRadius,
		const TransformComponent& characterTransform,
		const TransformedDisk& characterCollider);
}
