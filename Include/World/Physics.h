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

		struct UpdateBVHConfig
		{
			bool mForceRebuild{};
			bool mOnlyRebuildForNewColliders{};
			float mMaxAmountRefitBeforeRebuilding = 10'000.0f;
		};

		void UpdateBVHs(UpdateBVHConfig config = {});

		struct LineTraceResult
		{
			operator bool() const { return mHitEntity != entt::null; }

			float mDist = std::numeric_limits<float>::infinity();
			entt::entity mHitEntity = entt::null;

		private:
			friend ReflectAccess;
			static MetaType Reflect();
			REFLECT_AT_START_UP(LineTraceResult);
		};
		LineTraceResult LineTrace(const Line& line, const CollisionRules& filter) const;

		std::vector<entt::entity> FindAllWithinShape(const TransformedDisk& shape, const CollisionRules& filter) const;
		std::vector<entt::entity> FindAllWithinShape(const TransformedAABB& shape, const CollisionRules& filter) const;
		std::vector<entt::entity> FindAllWithinShape(const TransformedPolygon& shape, const CollisionRules& filter) const;

		// Low level API. Will return true if a blocking hit was found (if shouldReturn returned true), and searching was stopped.
		template<typename... CallbackAdditionalArgs>
		bool Query(const auto& inquirerShape,
			const CollisionRules& filter,
			const auto& onIntersect = BVH::DefaultOnIntersectFunction{},
			const auto& shouldCheck = BVH::DefaultShouldCheckFunction<true>{},
			const auto& shouldReturn = BVH::DefaultShouldReturnFunction<true>{},
			CallbackAdditionalArgs&&... args) const;

		using BVHS = std::array<BVH, static_cast<size_t>(CollisionLayer::NUM_OF_LAYERS)>;

		BVHS& GetBVHs() { return mBVHs; }
		const BVHS& GetBVHs() const { return mBVHs; }

		World& GetWorld() { return mWorld; }
		const World& GetWorld() const { return mWorld; }

	private:
		template<typename T>
		std::vector<entt::entity> FindAllWithinShapeImpl(const T& shape, const CollisionRules& filter) const;

		template<typename Collider, typename TransformedCollider>
		void UpdateTransformedColliders(World& world, std::array<bool, static_cast<size_t>(CollisionLayer::NUM_OF_LAYERS)>& wereItemsAddedToLayer);

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(Physics);

		std::reference_wrapper<World> mWorld;

		BVHS mBVHs;
	};
}

template <typename... CallbackAdditionalArgs>
bool CE::Physics::Query(const auto& inquirerShape,
	const CollisionRules& filter,
	const auto& onIntersect, 
	const auto& shouldCheck,
	const auto& shouldReturn, 
	CallbackAdditionalArgs&&... args) const
{
	for (const BVH& bvh : mBVHs)
	{
		if (filter.mResponses[static_cast<int>(bvh.GetLayer())] == CollisionResponse::Ignore)
		{
			continue;
		}

		if (bvh.Query(inquirerShape, 
			onIntersect,
			shouldCheck,
			shouldReturn,
			args...))
		{
			return true;
		}
	}
	return false;
}