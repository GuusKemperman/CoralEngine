#pragma once
#include <queue>

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

		using BVHS = std::array<BVH, magic_enum::enum_count<CollisionLayer>()>;

		BVHS& GetBVHs() { return mBVHs; }
		const BVHS& GetBVHs() const { return mBVHs; }

		World& GetWorld() { return mWorld; }
		const World& GetWorld() const { return mWorld; }

		void Update(float deltaTime);

		void ApplyVelocities(float deltaTime);

		void SyncWorldToPhysics();

		struct UpdateBVHConfig
		{
			bool mForceRebuild{};
			bool mOnlyRebuildForNewColliders{};
			float mMaxAmountRefitBeforeRebuilding = 10'000.0f;
		};
		void UpdateBVHs(UpdateBVHConfig config = {});

		void ResolveCollisions();

		void DebugDraw(RenderCommandQueue& commandQueue) const;

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

		entt::entity GetNearest(glm::vec2 point, const CollisionRules& filter) const;
		std::vector<entt::entity> GetSortedNearToFar(glm::vec2 point, const CollisionRules& filter) const;

		entt::entity GetFarthest(glm::vec2 point, const CollisionRules& filter);
		std::vector<entt::entity> GetSortedFarToNear(glm::vec2 point, const CollisionRules& filter);
		
		float GetSignedDistance(entt::entity entity, glm::vec2 point) const;

		// Low level API. Will return true if a blocking hit was found (if shouldReturn returned true), and searching was stopped.
		template<typename OnIntersect = BVH::DefaultOnIntersectFunction,
			typename ShouldReturn = BVH::DefaultShouldReturnFunction<false>,
			typename ShouldCheck = BVH::DefaultShouldCheckFunction,
			typename... CallbackAdditionalArgs>
		bool Query(const auto& inquirerShape,
			const CollisionRules& filter,
			const OnIntersect& onIntersect = {},
			const ShouldReturn& shouldReturn = {},
			const ShouldCheck& shouldCheck = {},
			CallbackAdditionalArgs&&... args) const;

		enum class ExploreOrder : bool
		{
			NearestFirst,
			FarthestFirst,
		};

		struct DefaultOnExplore
		{
			template<typename... Args>
			constexpr void operator()([[maybe_unused]] entt::entity entity, [[maybe_unused]] float signedDist, entt::entity, Args...) const {}
		};

		struct ExploreDefaultShouldCheckFunction
		{
			template<typename... Args>
			constexpr bool operator()([[maybe_unused]] entt::entity entity, Args...) const { return true; }
		};

		template<bool AlwaysReturnValue>
		struct ExploreDefaultShouldReturnFunction
		{
			template<typename... Args>
			constexpr bool operator()([[maybe_unused]] entt::entity entity, [[maybe_unused]] float signedDist, Args...) const { return AlwaysReturnValue; }
		};

		template<typename OnExplore = DefaultOnExplore,
			typename ShouldReturn = ExploreDefaultShouldReturnFunction<false>,
			typename ShouldCheck = ExploreDefaultShouldCheckFunction,
			typename... CallbackAdditionalArgs>
		bool Explore(ExploreOrder order,
			glm::vec2 location,
			const CollisionRules& filter,
			const OnExplore& onExplore = {},
			const ShouldReturn& shouldReturn = {},
			const ShouldCheck& shouldCheck = {},
			CallbackAdditionalArgs&&... args) const;

		template<ExploreOrder Order,
			typename OnExplore = DefaultOnExplore,
			typename ShouldReturn = ExploreDefaultShouldReturnFunction<false>,
			typename ShouldCheck = ExploreDefaultShouldCheckFunction,
			typename... CallbackAdditionalArgs>
		bool Explore(glm::vec2 location,
			const CollisionRules& filter,
			const OnExplore& onExplore = {},
			const ShouldReturn& shouldReturn = {},
			const ShouldCheck& shouldCheck = {},
			CallbackAdditionalArgs&&... args) const;

	private:
		template<typename T>
		std::vector<entt::entity> FindAllWithinShapeImpl(const T& shape, const CollisionRules& filter) const;

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(Physics);

		std::reference_wrapper<World> mWorld;
		BVHS mBVHs;

		entt::storage_for_t<TransformedAABBColliderComponent>& mAABBsStorage;
		entt::storage_for_t<TransformedDiskColliderComponent>& mDiskssStorage;
		entt::storage_for_t<TransformedPolygonColliderComponent>& mPolysStorage;

		struct CollisionData
		{
			entt::entity mEntity1{};
			entt::entity mEntity2{};

			/// The penetration depth of the two physics bodies
			/// (before they were displaced to resolve overlap).
			float mDepth{};

			/// The normal vector on the point of contact, pointing away from entity2's physics body.
			glm::vec2 mNormalFor1{};

			/// The approximate point of contact of the collision, in world coordinates.
			glm::vec2 mContactPoint{};
		};
		std::vector<CollisionData> mPreviousCollisions{};

		static glm::vec2 ResolveDiskCollision(const CollisionData& collisionToResolve,
			const PhysicsBody2DComponent& bodyToMove,
			const PhysicsBody2DComponent& otherBody,
			float multiplicant = 1.0f);

		void RegisterCollision(std::vector<CollisionData>& currentCollisions,
			CollisionData& collision, entt::entity entity1, entt::entity entity2);

		static bool CollisionCheck(TransformedDiskColliderComponent disk1, TransformedDiskColliderComponent disk2, CollisionData& result);

		static bool CollisionCheck(TransformedDiskColliderComponent disk, const TransformedPolygonColliderComponent& polygon, CollisionData& result);

		static bool CollisionCheck(TransformedDiskColliderComponent disk, TransformedAABBColliderComponent aabb, CollisionData& result);

		template<typename CollisionDataContainer>
		void CallEvents(const CollisionDataContainer& collisions, const EventBase& eventBase);

		void CallEvent(const BoundEvent& event, entt::sparse_set& storage, 
			entt::entity owner, 
			entt::entity otherEntity, 
			float depth, 
			glm::vec2 normal, 
			glm::vec2 contactPoint);

		friend struct Physics2DUnitTestAccess;
	};
}

template <typename OnIntersect,
	typename ShouldReturn,
	typename ShouldCheck,
	typename ... CallbackAdditionalArgs>
bool CE::Physics::Query(const auto& inquirerShape,
	const CollisionRules& filter,
	const OnIntersect& onIntersect,
	const ShouldReturn& shouldReturn,
	const ShouldCheck& shouldCheck,
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
			shouldReturn,
			shouldCheck,
			args...))
		{
			return true;
		}
	}
	return false;
}

template <typename OnExplore,
	typename ShouldReturn,
	typename ShouldCheck,
	typename... CallbackAdditionalArgs>
bool CE::Physics::Explore(ExploreOrder order, 
	glm::vec2 location, 
	const CollisionRules& filter, 
	const OnExplore& onExplore,
	const ShouldReturn& shouldReturn, 
	const ShouldCheck& shouldCheck, 
	CallbackAdditionalArgs&&... args) const
{
	if (order == ExploreOrder::NearestFirst)
	{
		return Explore<ExploreOrder::NearestFirst>(location, filter, onExplore, shouldReturn, shouldCheck, std::forward<CallbackAdditionalArgs>(args)...);
	}
	return Explore<ExploreOrder::FarthestFirst>(location, filter, onExplore, shouldReturn, shouldCheck, std::forward<CallbackAdditionalArgs>(args)...);
}

template <CE::Physics::ExploreOrder Order,
	typename OnExplore,
	typename ShouldReturn,
	typename ShouldCheck,
	typename... CallbackAdditionalArgs>
bool CE::Physics::Explore(glm::vec2 location, 
	const CollisionRules& filter, 
	const OnExplore& onExplore,
	const ShouldReturn& shouldReturn, 
	const ShouldCheck& shouldCheck, 
	CallbackAdditionalArgs&&... args) const
{
	struct SignedDistEntry
	{
		float mSignedDist{};

		bool operator<(const SignedDistEntry& other) const
		{
			if constexpr (Order == ExploreOrder::NearestFirst)
			{
				return mSignedDist > other.mSignedDist;
			}
			else
			{
				return mSignedDist < other.mSignedDist;
			}
		}
	};

	struct BVHNodeEntry : SignedDistEntry
	{
		CollisionLayer mFromBVHOfLayer{};
		const BVH::Node* mNode{};
	};

	auto getNodeDist = [=](const BVH::Node& node)
		{
			if constexpr (Order == ExploreOrder::NearestFirst)
			{
				return node.mBoundingBox.SignedDistance(location);
			}
			else
			{
				// todo optimise


				return glm::sqrt(glm::max(
					glm::max(glm::distance2(node.mBoundingBox.mMin, location), glm::distance2(node.mBoundingBox.mMax, location)),
					glm::max(glm::distance2(glm::vec2{ node.mBoundingBox.mMin.x, node.mBoundingBox.mMax.y }, location),
						glm::distance2(glm::vec2{ node.mBoundingBox.mMax.x, node.mBoundingBox.mMin.y }, location))));
			}
		};

	struct EntityEntry : SignedDistEntry
	{
		entt::entity mEntity{};
	};

	struct Entry
	{
		std::variant<BVHNodeEntry, EntityEntry> mVariant;

		bool operator<(const Entry& other) const
		{
			return std::visit(
				[&](const auto& entry)
				{
					const SignedDistEntry& signedEntry = static_cast<const SignedDistEntry&>(entry);

					return std::visit(
						[&](const auto& otherEntry)
						{
							const SignedDistEntry& otherSignedEntry = static_cast<const SignedDistEntry&>(otherEntry);
							return signedEntry < otherSignedEntry;
						}, other.mVariant);
				},
				mVariant);
		}
	};

	std::priority_queue<Entry> queue{};

	for (const BVH& bvh : mBVHs)
	{
		if (filter.mResponses[static_cast<int>(bvh.GetLayer())] == CollisionResponse::Ignore
			|| bvh.mIsEmpty)
		{
			continue;
		}

		const BVH::Node& node = bvh.mNodes.front();
		queue.push(Entry{ BVHNodeEntry{ SignedDistEntry{ getNodeDist(node) }, bvh.GetLayer(), &node}});
	}

	while (!queue.empty())
	{
		Entry topEntry = queue.top();
		queue.pop();

		if (std::holds_alternative<BVHNodeEntry>(topEntry.mVariant))
		{
			const BVHNodeEntry& bvhNodeEntry = std::get<BVHNodeEntry>(topEntry.mVariant);
			const BVH::Node* node = bvhNodeEntry.mNode;
			const CollisionLayer layer = bvhNodeEntry.mFromBVHOfLayer;

			const BVH& bvh = mBVHs[static_cast<int>(layer)];

			if (node->mTotalNumOfObjects == 0)
			{
				const BVH::Node& child1 = bvh.mNodes[node->mStartIndex];
				const BVH::Node& child2 = bvh.mNodes[node->mStartIndex + 1];

				queue.push(Entry{ BVHNodeEntry{ SignedDistEntry{ getNodeDist(child1) }, layer, &child1 } });
				queue.push(Entry{ BVHNodeEntry{ SignedDistEntry{ getNodeDist(child2) }, layer, &child2 } });
				continue;
			}

			uint32 indexOfId = node->mStartIndex;

			const auto checkNode = [&]<typename T>(uint32 num)
			{
				for (uint32 i = 0; i < num; i++, indexOfId++)
				{
					const entt::entity owner = bvh.mIds[indexOfId];

					if (!shouldCheck(owner, args...))
					{
						continue;
					}

					const T* collider = bvh.TryGetCollider<T>(owner);

					if (collider == nullptr)
					{
						continue;
					}

					queue.push(Entry{ EntityEntry{ SignedDistEntry{ collider->SignedDistance(location) }, owner } });
				}
			};

			const uint32 numPolygons = node->mTotalNumOfObjects - node->mNumOfAABBS - node->mNumOfCircles;

			checkNode.template operator()<TransformedAABBColliderComponent>(node->mNumOfAABBS);
			checkNode.template operator()<TransformedDiskColliderComponent>(node->mNumOfCircles);
			checkNode.template operator()<TransformedPolygonColliderComponent>(numPolygons);
		}
		else
		{
			const EntityEntry& entry = std::get<EntityEntry>(topEntry.mVariant);

			onExplore(entry.mEntity, entry.mSignedDist, args...);
			
			if (shouldReturn(entry.mEntity, entry.mSignedDist, args...))
			{
				return true;
			}
		}
	}

	return false;
}

