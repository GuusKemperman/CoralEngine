#pragma once
#include "Meta/MetaReflect.h"

namespace CE
{
	class World;
	class TransformComponent;
	struct TransformedDisk;
	struct TransformedAABB;
	struct TransformedPolygon;
	struct CollisionRules;
	class BVH;

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

		BVH& GetBVH() { return *mBVH; }
		const BVH& GetBVH() const { return *mBVH; }

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

		std::unique_ptr<BVH> mBVH{};
	};
}
