#pragma once
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Utilities/BVH.h"
#include "Meta/MetaReflect.h"

namespace CE
{
	class World;
	class TransformComponent;
	struct TransformedDisk;
	struct TransformedAABB;
	struct TransformedPolygon;
	struct CollisionRules;

	/**
	 * \brief Stores the physics-related data to allow for faster queries.
	 */
	class Physics
	{
	public:
		Physics(World& world);
		~Physics();

		Physics(Physics&&) = delete;
		Physics(const Physics&) = delete;

		Physics& operator=(Physics&&) = delete;
		Physics& operator=(const Physics&) = delete;

		std::vector<entt::entity> FindAllWithinShape(const TransformedDisk& shape, const CollisionRules& filter) const;
		std::vector<entt::entity> FindAllWithinShape(const TransformedAABB& shape, const CollisionRules& filter) const;
		std::vector<entt::entity> FindAllWithinShape(const TransformedPolygon& shape, const CollisionRules& filter) const;

		using BVHS = std::array<BVH, static_cast<size_t>(CollisionLayer::NUM_OF_LAYERS)>;

		BVHS& GetBVHs() { return mBVHs; }
		const BVHS& GetBVHs() const { return mBVHs; }

		World& GetWorld() { return mWorld; }
		const World& GetWorld() const { return mWorld; }

	private:
		template<typename T>
		std::vector<entt::entity> FindAllWithinShapeImpl(const T& shape, const CollisionRules& filter) const;

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(Physics);

		// mWorld needs to be updated in World::World(World&&), so we give access to World to do so.
		friend class World;
		std::reference_wrapper<World> mWorld;

		BVHS mBVHs;
	};
}
