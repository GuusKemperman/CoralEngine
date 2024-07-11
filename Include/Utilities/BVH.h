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

		template<bool AlwaysReturnValue>
		struct DefaultShouldCheckFunction
		{
			template<typename TransformedColliderType, typename ...CallbackAdditionalArgs>
			static bool Callback(CallbackAdditionalArgs&& ...) { return AlwaysReturnValue; }
		};

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

		template<typename OnIntersectFunction = DefaultOnIntersectFunction, typename ShouldCheckFunction = DefaultShouldCheckFunction<true>, typename ShouldReturnFunction = DefaultShouldReturnFunction<true>, typename InquirerShape, typename ...CallbackAdditionalArgs>
		bool Query(const InquirerShape inquirerShape, CallbackAdditionalArgs&& ...args) const;

		void DebugDraw() const;

		CollisionLayer GetLayer() const { return mLayer; }

		float GetAmountRefitted() const { return mAmountRefitted; }

	private:
		const Registry& GetRegistry() const;

		struct Node
		{
			// Some of our overlap functions expect a 'valid'
			// bounding box. This means we cant use std::numeric_limits<float>::infinity(),
			// as some operations we use create NaNs.
			static constexpr float sLargeNum = 123456789101112131415.0f;
			TransformedAABB mBoundingBox{ glm::vec2{ -sLargeNum }, glm::vec2{ -sLargeNum + 1.0f} };
			uint32 mStartIndex{};
			uint32 mTotalNumOfObjects{}; // The number of polygons can be extracted since nunOfPolygons = total - aabbs - circles
			uint32 mNumOfAABBS{};
			uint32 mNumOfCircles{};
		};
		static_assert(sizeof(Node) == 32);

		float UpdateNodeBounds(Node& node);
		void Subdivide(Node& node);

		template<typename OnIntersectFunction, typename ShouldReturnFunction, typename InquirerShapeType, typename ObjectShapeType, typename ...CallbackAdditionalArgs>
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

		bool mEmpty = true;
		float mAmountRefitted{};
	};

	template <typename OnIntersectFunction, typename ShouldCheckFunction, typename ShouldReturnFunction, typename
		InquirerShape, typename ... CallbackAdditionalArgs>
	bool BVH::Query(const InquirerShape inquirerShape, CallbackAdditionalArgs&&... args) const
	{
		static constexpr uint32 stackSize = 256;
		const Node* stack[stackSize];
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
						ASSERT(stackPtr + 1 < stackSize);
						stack[stackPtr++] = child2;
					}
				}
				continue;
			}

			uint32 indexOfId = node->mStartIndex;

			for (uint32 i = 0; i < node->mNumOfAABBS; i++, indexOfId++)
			{
				const entt::entity owner = mIds[indexOfId];

				if(!ShouldCheckFunction::template Callback<TransformedAABBColliderComponent>(owner, std::forward<CallbackAdditionalArgs>(args)...))
				{
					continue;
				}

				const TransformedAABB* aabb = reg.TryGet<TransformedAABBColliderComponent>(owner);

				if (aabb == nullptr)
				{
					continue;
				}

				if (TestAgainstObject<OnIntersectFunction, ShouldReturnFunction>(inquirerShape, *aabb, owner, std::forward<CallbackAdditionalArgs>(args)...))
				{
					return true;
				}
			}

			for (uint32 i = 0; i < node->mNumOfCircles; i++, indexOfId++)
			{
				const entt::entity owner = mIds[indexOfId];

				if (!ShouldCheckFunction::template Callback<TransformedDiskColliderComponent>(owner, std::forward<CallbackAdditionalArgs>(args)...))
				{
					continue;
				}

				const TransformedDisk* circle = reg.TryGet<TransformedDiskColliderComponent>(owner);

				if (circle == nullptr)
				{
					continue;
				}

				if (TestAgainstObject<OnIntersectFunction, ShouldReturnFunction>(inquirerShape, *circle, owner, std::forward<CallbackAdditionalArgs>(args)...))
				{
					return true;
				}
			}

			const uint32 numOfPolygons = node->mTotalNumOfObjects - node->mNumOfAABBS - node->mNumOfCircles;
			for (uint32 i = 0; i < numOfPolygons; i++, indexOfId++)
			{
				const entt::entity owner = mIds[indexOfId];

				if (!ShouldCheckFunction::template Callback<TransformedPolygonColliderComponent>(owner, std::forward<CallbackAdditionalArgs>(args)...))
				{
					continue;
				}

				const TransformedPolygon* polygon = reg.TryGet<TransformedPolygonColliderComponent>(owner);

				if (polygon == nullptr)
				{
					continue;
				}

				if (TestAgainstObject<OnIntersectFunction, ShouldReturnFunction>(inquirerShape, *polygon, owner, std::forward<CallbackAdditionalArgs>(args)...))
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

	template <typename OnIntersectFunction, typename ShouldReturnFunction, typename
		InquirerShapeType, typename ObjectShapeType, typename ... CallbackAdditionalArgs>
	bool BVH::TestAgainstObject(const InquirerShapeType inquirerShape, const ObjectShapeType& object,
		entt::entity owner, CallbackAdditionalArgs&&... args)
	{
		if (!AreOverlapping(object, inquirerShape))
		{
			return false;
		}

		OnIntersectFunction:: Callback(object, owner, std::forward<CallbackAdditionalArgs>(args)...);
		return ShouldReturnFunction:: Callback(object, owner, std::forward<CallbackAdditionalArgs>(args)...);
	}
}
