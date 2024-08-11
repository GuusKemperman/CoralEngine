#include "Precomp.h"
#include "Utilities/BVH.h"

#include "Components/TransformComponent.h"
#include "Components/Physics2D/AABBColliderComponent.h"
#include "Components/Physics2D/DiskColliderComponent.h"
#include "Components/Physics2D/PolygonColliderComponent.h"
#include "World/Registry.h"
#include "Utilities/DrawDebugHelpers.h"
#include "World/Physics.h"

CE::BVH::BVH(Physics& physics, CollisionLayer layer) :
    mPhysics(&physics),
    mLayer(layer)
{
    mNodes.resize(4);
}

void CE::BVH::Build()
{
    // Paranoia
    memset(mNodes.data(), 0, mNodes.capacity() * sizeof(mNodes[0]));
    memset(mIds.data(), 0, mIds.capacity() * sizeof(mIds[0]));

    mNodes.clear();
    mIds.clear();
    mAmountRefitted = 0.0f;

    const Registry& reg = mPhysics->GetWorld().GetRegistry();

    const auto aabbView = reg.View<PhysicsBody2DComponent, TransformedAABBColliderComponent>();
    const auto circlesView = reg.View<PhysicsBody2DComponent, TransformedDiskColliderComponent>();
    const auto polygonView = reg.View<PhysicsBody2DComponent, TransformedPolygonColliderComponent>();

    for (const entt::entity entity : aabbView)
    {
        if (aabbView.get<PhysicsBody2DComponent>(entity).mRules.mLayer == mLayer)
        {
            mIds.emplace_back(entity);
        }
    }

    const uint32 numOfAABBs = static_cast<uint32>(mIds.size());

    for (const entt::entity entity : circlesView)
    {
        if (circlesView.get<PhysicsBody2DComponent>(entity).mRules.mLayer == mLayer)
        {
            mIds.emplace_back(entity);
        }
    }

    const uint32 numOfCircles = static_cast<uint32>(mIds.size()) - numOfAABBs;

    for (const entt::entity entity : polygonView)
    {
        if (polygonView.get<PhysicsBody2DComponent>(entity).mRules.mLayer == mLayer)
        {
            mIds.emplace_back(entity);
        }
    }

    const uint32 numOfPolygons = static_cast<uint32>(mIds.size()) - numOfAABBs - numOfCircles;
    const uint32 totalNumObjects = numOfAABBs + numOfCircles + numOfPolygons;

    if (totalNumObjects != 0)
    {
        mIds.resize(totalNumObjects);

        size_t maxSize = (2 * totalNumObjects - 1) * 2;

        if (mNodes.capacity() < maxSize)
        {
            mNodes.reserve(maxSize + maxSize / 2);
        }
    }

    Node& root = mNodes.emplace_back();
    root.mStartIndex = 0;

    root.mNumOfAABBS = numOfAABBs;
    root.mNumOfCircles = numOfCircles;
    root.mTotalNumOfObjects = totalNumObjects;

    mEmpty = root.mTotalNumOfObjects == 0;

    if (mEmpty)
    {
        mNodes.resize(4);
        return;
    }

    // Empty dummy node, is just
    // there for cache alignment.
    mNodes.emplace_back();

    UpdateNodeBounds(root);

    if (root.mTotalNumOfObjects > 2)
    {
        Subdivide(root);
    }
}

void CE::BVH::Refit()
{
    if (mEmpty)
    {
        return;
    }

    for (int i = static_cast<int>(mNodes.size()) - 1; i >= 0; i--)
    {
        if (i == 1)
        {
            continue;
        }

        Node& node = mNodes[i];
        if (node.mTotalNumOfObjects != 0)
        {
            // leaf node: adjust bounds to contained triangles
            mAmountRefitted += UpdateNodeBounds(node);
            continue;
        }

        // interior node: adjust bounds to child node bounds
        Node& leftChild = mNodes[node.mStartIndex];
        Node& rightChild = mNodes[node.mStartIndex + 1];
        node.mBoundingBox.mMin = glm::min(leftChild.mBoundingBox.mMin, rightChild.mBoundingBox.mMin);
        node.mBoundingBox.mMax = glm::max(leftChild.mBoundingBox.mMax, rightChild.mBoundingBox.mMax);
    }
}

void CE::BVH::DebugDraw() const
{
    if (mEmpty)
    {
        return;
    }

    for (int i = static_cast<int>(mNodes.size()) - 1; i >= 0; i--)
    {
        if (i == 1)
        {
            continue;
        }

        const Node& node = mNodes[i];
        if (node.mTotalNumOfObjects == 0)
        {
            AddDebugBox(mPhysics->GetWorld().GetRenderCommandQueue(), 
                DebugDraw::AccelStructs, 
                To3D(node.mBoundingBox.GetCentre()), 
                To3D(node.mBoundingBox.GetSize() * .5f), glm::vec4{ 0.0f, 1.0f, 1.0f, 1.0f });
        }
    }
}

const CE::Registry& CE::BVH::GetRegistry() const
{
    return mPhysics->GetWorld().GetRegistry();
}

float CE::BVH::UpdateNodeBounds(Node& node)
{
    const TransformedAABB initialAABB = node.mBoundingBox;

    node.mBoundingBox.mMin = glm::vec2(INFINITY);
    node.mBoundingBox.mMax = glm::vec2(-INFINITY);

    uint32 indexOfId = node.mStartIndex;
    const Registry& reg = mPhysics->GetWorld().GetRegistry();

    for (uint32 i = 0; i < node.mNumOfAABBS; i++, indexOfId++)
    {
        const entt::entity owner = mIds[indexOfId];
        const TransformedAABB* const aabb = reg.TryGet<TransformedAABBColliderComponent>(owner);

        if (aabb != nullptr)
        {
    		    node.mBoundingBox.CombineWith(*aabb);
        }
    }

    for (uint32 i = 0; i < node.mNumOfCircles; i++, indexOfId++)
    {
        const entt::entity owner = mIds[indexOfId];
        const TransformedDisk* const circle = reg.TryGet<TransformedDiskColliderComponent>(owner);

        if (circle != nullptr)
        {
			      node.mBoundingBox.CombineWith(circle->GetBoundingBox());
        }
    }

    const uint32 numOfPolygons = node.mTotalNumOfObjects - node.mNumOfAABBS - node.mNumOfCircles;
    for (uint32 i = 0; i < numOfPolygons; i++, indexOfId++)
    {
        const entt::entity owner = mIds[indexOfId];
        const TransformedPolygon* const polygon = reg.TryGet<TransformedPolygonColliderComponent>(owner);

        if (polygon != nullptr)
        {
            node.mBoundingBox.CombineWith(polygon->GetBoundingBox());
        }
    }

    return glm::distance2(initialAABB.mMin, node.mBoundingBox.mMin) + glm::distance2(initialAABB.mMax, node.mBoundingBox.mMax);
}

void CE::BVH::Subdivide(Node& node)
{
    const SplitPoint splitPoint = DetermineSplitPos(node);

    static std::vector<entt::entity> childrenIds[2]{};
    childrenIds[0].clear();
    childrenIds[1].clear();
    childrenIds[0].reserve(node.mTotalNumOfObjects);
    childrenIds[1].reserve(node.mTotalNumOfObjects);

    ASSERT(mNodes.size() + 2 <= mNodes.capacity());
    uint32 firstNodeIndex = static_cast<uint32>(mNodes.size());
    mNodes.emplace_back();
    mNodes.emplace_back();

    Node* children[2]
    {
        &mNodes[firstNodeIndex],
        &mNodes[firstNodeIndex + 1]
    };

    for (uint32 i = 0; i < 2; i++)
    {
        children[i]->mNumOfAABBS = children[i]->mNumOfCircles = 0;
    }

    const Registry& reg = mPhysics->GetWorld().GetRegistry();
    uint32 indexOfId = node.mStartIndex;
    static bool edgeCaseFlipper{};

    for (uint32 i = 0; i < node.mNumOfAABBS; i++, indexOfId++)
    {
        const entt::entity owner = mIds[indexOfId];
        const TransformedAABB& collider = reg.Get<TransformedAABBColliderComponent>(owner);

        const glm::vec2 centre = collider.GetCentre();
        const float posOnAxis = centre[splitPoint.mAxis];
        bool childIndex = posOnAxis < splitPoint.mPosition;

        if (posOnAxis == splitPoint.mPosition)
        {
            childIndex = edgeCaseFlipper;
            edgeCaseFlipper = !edgeCaseFlipper;
        }

        childrenIds[childIndex].push_back(owner);
        children[childIndex]->mNumOfAABBS++;
    }

    for (uint32 i = 0; i < node.mNumOfCircles; i++, indexOfId++)
    {
        const entt::entity owner = mIds[indexOfId];
        const TransformedDisk& collider = reg.Get<TransformedDiskColliderComponent>(owner);

        const glm::vec2 centre = collider.GetCentre();
        const float posOnAxis = centre[splitPoint.mAxis];
        bool childIndex = posOnAxis < splitPoint.mPosition;

        if (posOnAxis == splitPoint.mPosition)
        {
            childIndex = edgeCaseFlipper;
            edgeCaseFlipper = !edgeCaseFlipper;
        }

        childrenIds[childIndex].push_back(owner);
        children[childIndex]->mNumOfCircles++;
    }

    const uint32 numOfPolygons = node.mTotalNumOfObjects - node.mNumOfAABBS - node.mNumOfCircles;
    for (uint32 i = 0; i < numOfPolygons; i++, indexOfId++)
    {
        const entt::entity owner = mIds[indexOfId];
        const TransformedPolygon& polygon = reg.Get<TransformedPolygonColliderComponent>(owner);

        const glm::vec2 centre = polygon.mBoundingBox.GetCentre();
        const float posOnAxis = centre[splitPoint.mAxis];
        bool childIndex = posOnAxis < splitPoint.mPosition;

        if (posOnAxis == splitPoint.mPosition)
        {
            childIndex = edgeCaseFlipper;
            edgeCaseFlipper = !edgeCaseFlipper;
        }

        childrenIds[childIndex].push_back(owner);
    }

    children[0]->mTotalNumOfObjects = static_cast<uint32>(childrenIds[0].size());
    children[1]->mTotalNumOfObjects = static_cast<uint32>(childrenIds[1].size());

    children[0]->mStartIndex = node.mStartIndex;
    children[1]->mStartIndex = node.mStartIndex + children[0]->mTotalNumOfObjects;

    for (uint32 childIndex = 0; childIndex < 2; childIndex++)
    {
        uint32 idIndex = children[childIndex]->mStartIndex;
        for (uint32 j = 0; j < children[childIndex]->mTotalNumOfObjects; j++, idIndex++)
        {
            mIds[idIndex] = childrenIds[childIndex][j];
        }
    }

    node.mStartIndex = firstNodeIndex;
    node.mTotalNumOfObjects = 0;

    for (uint32 i = 0; i < 2; i++)
    {
        UpdateNodeBounds(*children[i]);

        if (children[i]->mTotalNumOfObjects > 2)
        {
            Subdivide(*children[i]);
        }
        else
        {
            if (children[i]->mTotalNumOfObjects == 0)
            {
                children[i]->mBoundingBox.mMin = { -INFINITY, -INFINITY };
                children[i]->mBoundingBox.mMax = { -INFINITY, -INFINITY };
            }
        }
    }
}

float CE::BVH::DetermineSplitPointCost(const Node& node, SplitPoint splitPoint) const
{
    TransformedAABB boxes[2]
    {
        { glm::vec2{INFINITY}, glm::vec2{-INFINITY}},
        { glm::vec2{INFINITY}, glm::vec2{-INFINITY}},
    };
    uint32 amountOfObjects[2]{};

    uint32 indexOfId = node.mStartIndex;

    const Registry& reg = mPhysics->GetWorld().GetRegistry();

    for (uint32 i = 0; i < node.mNumOfAABBS; i++, indexOfId++)
    {
        const entt::entity owner = mIds[indexOfId];
        const TransformedAABB& collider = reg.Get<TransformedAABBColliderComponent>(owner);

        const glm::vec2 centre = collider.GetCentre();
        const float posOnAxis = centre[splitPoint.mAxis];
        const bool childIndex = posOnAxis < splitPoint.mPosition;

        amountOfObjects[childIndex]++;
        boxes[childIndex].CombineWith(collider);
    }

    for (uint32 i = 0; i < node.mNumOfCircles; i++, indexOfId++)
    {
        const entt::entity owner = mIds[indexOfId];
        const TransformedDisk& collider = reg.Get<TransformedDiskColliderComponent>(owner);

        const glm::vec2 centre = collider.GetCentre();
        const float posOnAxis = centre[splitPoint.mAxis];
        const bool childIndex = posOnAxis < splitPoint.mPosition;

        amountOfObjects[childIndex]++;
        boxes[childIndex].CombineWith(collider.GetBoundingBox());
    }

    const uint32 numOfPolygons = node.mTotalNumOfObjects - node.mNumOfAABBS - node.mNumOfCircles;
    for (uint32 i = 0; i < numOfPolygons; i++, indexOfId++)
    {
        const entt::entity owner = mIds[indexOfId];
        const TransformedPolygon& polygon = reg.Get<TransformedPolygonColliderComponent>(owner);

        const glm::vec2 centre = polygon.GetBoundingBox().GetCentre();
        const float posOnAxis = centre[splitPoint.mAxis];
        const bool childIndex = posOnAxis < splitPoint.mPosition;

        amountOfObjects[childIndex]++;
        boxes[childIndex].CombineWith(polygon.GetBoundingBox());
    }

    const float cost = amountOfObjects[0] * boxes[0].GetPerimeter() + amountOfObjects[1] * boxes[1].GetPerimeter();
    return cost > 0 ? cost : 1e30f;
}

CE::BVH::SplitPoint CE::BVH::DetermineSplitPos(const Node& node)
{
    TransformedAABB centroidsBoundingBox = { glm::vec2{INFINITY}, glm::vec2{-INFINITY} };

    const Registry& reg = mPhysics->GetWorld().GetRegistry();
    uint32 indexOfId = node.mStartIndex;
    for (uint32 i = 0; i < node.mNumOfAABBS; i++, indexOfId++)
    {
        const entt::entity owner = mIds[indexOfId];
        const TransformedAABB& collider = reg.Get<TransformedAABBColliderComponent>(owner);

        const glm::vec2 centre = collider.GetCentre();

        centroidsBoundingBox.mMin.x = glm::min(centroidsBoundingBox.mMin.x, centre.x);
        centroidsBoundingBox.mMin.y = glm::min(centroidsBoundingBox.mMin.y, centre.y);
        centroidsBoundingBox.mMax.x = glm::max(centroidsBoundingBox.mMax.x, centre.x);
        centroidsBoundingBox.mMax.y = glm::max(centroidsBoundingBox.mMax.y, centre.y);
    }

    for (uint32 i = 0; i < node.mNumOfCircles; i++, indexOfId++)
    {
        const entt::entity owner = mIds[indexOfId];
        const TransformedDisk& collider = reg.Get<TransformedDiskColliderComponent>(owner);

        const glm::vec2 centre = collider.mCentre;

        centroidsBoundingBox.mMin.x = glm::min(centroidsBoundingBox.mMin.x, centre.x);
        centroidsBoundingBox.mMin.y = glm::min(centroidsBoundingBox.mMin.y, centre.y);
        centroidsBoundingBox.mMax.x = glm::max(centroidsBoundingBox.mMax.x, centre.x);
        centroidsBoundingBox.mMax.y = glm::max(centroidsBoundingBox.mMax.y, centre.y);
    }

    const uint32 numOfPolygons = node.mTotalNumOfObjects - node.mNumOfAABBS - node.mNumOfCircles;
    for (uint32 i = 0; i < numOfPolygons; i++, indexOfId++)
    {
        const entt::entity owner = mIds[indexOfId];
        const TransformedPolygon& polygon = reg.Get<TransformedPolygonColliderComponent>(owner);

        const glm::vec2 centre = polygon.mBoundingBox.GetCentre();
        centroidsBoundingBox.mMin.x = glm::min(centroidsBoundingBox.mMin.x, centre.x);
        centroidsBoundingBox.mMin.y = glm::min(centroidsBoundingBox.mMin.y, centre.y);
        centroidsBoundingBox.mMax.x = glm::max(centroidsBoundingBox.mMax.x, centre.x);
        centroidsBoundingBox.mMax.y = glm::max(centroidsBoundingBox.mMax.y, centre.y);
    }

    const glm::vec2 extent = centroidsBoundingBox.mMax - centroidsBoundingBox.mMin;

    SplitPoint currentPoint{}, bestPoint{};
    currentPoint.mAxis = extent.y > extent.x;

    float lowestCost = INFINITY;

    constexpr uint32 numOfSamples = 16;

    for (uint32 i = 1; i <= numOfSamples; i++)
    {
        const float asPercentage = static_cast<float>(i) * (1.0f / static_cast<float>(numOfSamples + 1));
        currentPoint.mPosition = centroidsBoundingBox.mMin[currentPoint.mAxis] + extent[currentPoint.mAxis] * asPercentage;

        const float cost = DetermineSplitPointCost(node, currentPoint);
        if (cost < lowestCost)
        {
            lowestCost = cost;
            bestPoint = currentPoint;
        }
    }

    return bestPoint;
}