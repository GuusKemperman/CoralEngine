#pragma once
#include "Geometry2d.h"
#include "Components/Physics2D/AABBColliderComponent.h"
#include "Components/Physics2D/DiskColliderComponent.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Components/Physics2D/PolygonColliderComponent.h"
#include "World/Registry.h"

namespace CE
{
	class World;

	class BVH
	{
	public:
		BVH() = default;
		BVH(Physics& physics, CollisionLayer layer);

		void Build();
		void Refit();

		template<typename TransformedColliderType>
		void Insert(const Span<entt::entity>& entities) = delete;

		template<>
		void Insert<TransformedDiskColliderComponent>(const Span<entt::entity>& entities);

		template<>
		void Insert<TransformedAABBColliderComponent>(const Span<entt::entity>& entities);

		template<>
		void Insert<TransformedPolygonColliderComponent>(const Span<entt::entity>& entities);

		template<bool AlwaysReturnValue>
		struct DefaultShouldReturnFunction
		{
			template<typename ...CallbackAdditionalArgs>
			static bool Callback(CallbackAdditionalArgs&& ...) { return AlwaysReturnValue; }
		};

		struct DefaultOnIntersectFunction
		{
			template<typename ...CallbackAdditionalArgs>
			static void Callback(CallbackAdditionalArgs&& ...) { }
		};

		template<typename OnIntersectFunction, typename ShouldCheckFunction, typename ShouldReturnFunction, typename InquirerShape, typename ...CallbackAdditionalArgs>
		bool Query(const InquirerShape inquirerShape, CallbackAdditionalArgs&& ...args) const;

		void DebugDraw() const;

		CollisionLayer GetLayer() const { return mLayer; }

		float GetRebuildDesire() const { return mAmountRefitted + static_cast<float>(mNodes[2].mTotalNumOfObjects) * 500.0f; }

		uint32 GetNumOfInsertedItems() const { return mNodes[2].mTotalNumOfObjects; }

	private:
		const Registry& GetRegistry() const;

		struct Node
		{
			TransformedAABB mBoundingBox{ { -INFINITY, -INFINITY }, { -INFINITY, -INFINITY } };
			uint32 mStartIndex{};
			uint32 mTotalNumOfObjects{}; // The number of polygons can be extracted since nunOfPolygons = total - aabbs - circles
			uint32 mNumOfAABBS{};
			uint32 mNumOfCircles{};
		};
		static_assert(sizeof(Node) == 32);

		float UpdateNodeBounds(Node& node);
		void Subdivide(Node& node);
		void DebugDraw(const Node& node) const;

		template<typename OnIntersectFunction, typename ShouldCheckFunction, typename ShouldReturnFunction, typename InquirerShapeType, typename ObjectShapeType, typename ...CallbackAdditionalArgs>
		static FORCE_INLINE bool TestAgainstObject(const InquirerShapeType inquirerShape, const ObjectShapeType& object, entt::entity owner, CallbackAdditionalArgs&& ...args);

		struct SplitPoint
		{
			uint32 mAxis{};
			float mPosition{};
		};
		float DetermineSplitPointCost(const Node& node, SplitPoint point) const;
		SplitPoint DetermineSplitPos(const Node& node);

		Physics* mPhysics;
		CollisionLayer mLayer{};

		std::vector<entt::entity> mIds{};
		std::vector<Node> mNodes{};

		// We make the assumption that if there is
		// any leaf node, that is has objects in it.
		// If it doesnt have objects, we assume its a
		// node with children. We have this boolean to
		// prevent an edge case when there are no objects
		// at all.
		bool mIsRootNodeEmpty = true;
		bool mIsInsertNodeEmpty = true;

		// The higher this is,
		// the more this BVH would
		// benefit from being rebuild.
		float mAmountRefitted{};
	};

	template <typename OnIntersectFunction, typename ShouldCheckFunction, typename ShouldReturnFunction, typename
		InquirerShape, typename ... CallbackAdditionalArgs>
	bool BVH::Query(const InquirerShape inquirerShape, CallbackAdditionalArgs&&... args) const
	{
		const Node* stack[64];
		uint32 stackPtr = 0;

		const Node* node = &mNodes[0];
		const Registry& reg = GetRegistry();

		while (1)
		{
			if (node->mTotalNumOfObjects == 0)
			{
				const Node* child1 = &mNodes[node->mStartIndex];
				const Node* child2 = &mNodes[node->mStartIndex + 1];

				bool intersect1 = AreOverlapping(child1->mBoundingBox, inquirerShape);
				bool intersect2 = AreOverlapping(child2->mBoundingBox, inquirerShape);

				if (intersect1 < intersect2)
				{
					// Swap them, using the node as a temporary
					node = child2;
					child2 = child1;
					child1 = node;

					std::swap(intersect1, intersect2);
				}

				// If this is false, we know intersect2 is also false
				if (!intersect1)
				{
					if (stackPtr == 0)
					{
						break;
					}
					node = stack[--stackPtr];
				}
				else
				{
					node = child1;
					if (intersect2)
					{
						stack[stackPtr++] = child2;
					}
				}
				continue;
			}

			uint32 indexOfId = node->mStartIndex;

			for (uint32 i = 0; i < node->mNumOfAABBS; i++, indexOfId++)
			{
				const entt::entity owner = mIds[indexOfId];
				const TransformedAABB* aabb = reg.TryGet<TransformedAABBColliderComponent>(owner);

				if (aabb == nullptr)
				{
					continue;
				}

				if (TestAgainstObject<OnIntersectFunction, ShouldCheckFunction, ShouldReturnFunction>(inquirerShape, *aabb, owner, std::forward<CallbackAdditionalArgs>(args)...))
				{
					return true;
				}
			}

			for (uint32 i = 0; i < node->mNumOfCircles; i++, indexOfId++)
			{
				const entt::entity owner = mIds[indexOfId];
				const TransformedDisk* circle = reg.TryGet<TransformedDiskColliderComponent>(owner);

				if (circle == nullptr)
				{
					continue;
				}

				if (TestAgainstObject<OnIntersectFunction, ShouldCheckFunction, ShouldReturnFunction>(inquirerShape, *circle, owner, std::forward<CallbackAdditionalArgs>(args)...))
				{
					return true;
				}
			}

			const uint32 numOfPolygons = node->mTotalNumOfObjects - node->mNumOfAABBS - node->mNumOfCircles;
			for (uint32 i = 0; i < numOfPolygons; i++, indexOfId++)
			{
				const entt::entity owner = mIds[indexOfId];
				const TransformedPolygon* polygon = reg.TryGet<TransformedPolygonColliderComponent>(owner);

				if (polygon == nullptr)
				{
					continue;
				}

				if (TestAgainstObject<OnIntersectFunction, ShouldCheckFunction, ShouldReturnFunction>(inquirerShape, *polygon, owner, std::forward<CallbackAdditionalArgs>(args)...))
				{
					return true;
				}
			}

			if (stackPtr == 0)
			{
				break;
			}
			node = stack[--stackPtr];
		}

		return false;
	}

	template <typename OnIntersectFunction, typename ShouldCheckFunction, typename ShouldReturnFunction, typename
		InquirerShapeType, typename ObjectShapeType, typename ... CallbackAdditionalArgs>
	bool BVH::TestAgainstObject(const InquirerShapeType inquirerShape, const ObjectShapeType& object,
		entt::entity owner, CallbackAdditionalArgs&&... args)
	{
		if (!ShouldCheckFunction::template Callback(object, owner, std::forward<CallbackAdditionalArgs>(args)...) 
			|| !AreOverlapping(object, inquirerShape))
		{
			return false;
		}

		OnIntersectFunction::template Callback(object, owner, std::forward<CallbackAdditionalArgs>(args)...);
		return ShouldReturnFunction::template Callback(object, owner, std::forward<CallbackAdditionalArgs>(args)...);
	}
}
