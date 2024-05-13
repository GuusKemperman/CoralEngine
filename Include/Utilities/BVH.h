#pragma once
#include "Geometry2d.h"
#include "Components/Physics2D/AABBColliderComponent.h"
#include "Components/Physics2D/DiskColliderComponent.h"
#include "Components/Physics2D/PolygonColliderComponent.h"
#include "World/Registry.h"

namespace CE
{
	class World;

	class BVH
	{
	public:
		BVH(Physics& physics);

		void Build();
		void Refit();

		template<bool AlwaysReturnValue>
		struct DefaultShouldReturnFunction
		{
			template<typename TransformedShapeType, typename ...CallbackAdditionalArgs>
			static inline bool Callback(entt::entity, CallbackAdditionalArgs&& ...) { return AlwaysReturnValue; }
		};

		struct DefaultOnIntersectFunction
		{
			template<typename TransformedShapeType, typename ...CallbackAdditionalArgs>
			static inline void Callback(entt::entity, CallbackAdditionalArgs&& ...) { }
		};

		template<typename OnIntersectFunction, typename ShouldReturnFunction, typename InquirerShape, typename ...CallbackAdditionalArgs>
		inline bool Query(const InquirerShape inquirerShape, CallbackAdditionalArgs&& ...args) const
		{
			const Node* stack[64];
			uint32 stackPtr = 0;

			const Node* node = &mNodes[0];
			const Registry& reg = mPhysics.get().GetWorld().GetRegistry();

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

					if (TestAgainstObject<OnIntersectFunction, ShouldReturnFunction>(inquirerShape, *aabb, owner, std::forward<CallbackAdditionalArgs>(args)...))
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

					if (TestAgainstObject<OnIntersectFunction, ShouldReturnFunction>(inquirerShape, *circle, owner, std::forward<CallbackAdditionalArgs>(args)...))
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

		void DebugDraw() const;

	private:
		struct Node
		{
			TransformedAABB mBoundingBox{ { -INFINITY, -INFINITY }, { -INFINITY, -INFINITY } };
			uint32 mStartIndex{};
			uint32 mTotalNumOfObjects{}; // The number of polygons can be extracted since nunOfPolygons = total - aabbs - circles
			uint32 mNumOfAABBS{};
			uint32 mNumOfCircles{};
		};
		static_assert(sizeof(Node) == 32);

		void UpdateNodeBounds(Node& node);
		void Subdivide(Node& node);
		void DebugDraw(const Node& node) const;

		template<typename OnIntersectFunction, typename ShouldReturnFunction, typename InquirerShapeType, typename ObjectShapeType, typename ...CallbackAdditionalArgs>
		static __forceinline bool TestAgainstObject(const InquirerShapeType inquirerShape, const ObjectShapeType object, CallbackAdditionalArgs&& ...args)
		{
			if (!AreOverlapping(object, inquirerShape))
			{
				return false;
			}

			OnIntersectFunction::template Callback<ObjectShapeType>(std::forward<CallbackAdditionalArgs>(args)...);
			return ShouldReturnFunction::template Callback<ObjectShapeType>(std::forward<CallbackAdditionalArgs>(args)...);
		}

		struct SplitPoint
		{
			uint32 mAxis{};
			float mPosition{};
		};
		float DetermineSplitPointCost(const Node& node, SplitPoint point) const;
		SplitPoint DetermineSplitPos(const Node& node);

		std::reference_wrapper<Physics> mPhysics;
		std::vector<entt::entity> mIds{};
		std::vector<Node> mNodes{};

		bool mEmpty = true;
	};
}
