#include "Precomp.h"
#include "Components/Pathfinding/NavMeshComponent.h"

#include <sstream>
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244)
#pragma warning(disable: 4267)
#endif

#include "cdt/CDT.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "clipper2/clipper.h"
#include "Components/Physics2D/AABBColliderComponent.h"
#include "Components/Physics2D/DiskColliderComponent.h"
#include "Components/Physics2D/PolygonColliderComponent.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Components/TransformComponent.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"
#include "World/Registry.h"
#include "World/World.h"
#include "Utilities/DrawDebugHelpers.h"
#include "World/Physics.h"

void CE::NavMeshComponent::GenerateNavMesh(const World& world)
{
	mPolygonDataNavMesh.clear();
	mCleanedPolygonList.clear();
	mAStarGraph = {};

	NavMeshData navMeshData = GenerateNavMeshData(world);
	mCleanedPolygonList = GetDifferences(navMeshData);
	Triangulation(mCleanedPolygonList);
	mNavMeshNeedsUpdate = false;

	mBVHWorld.emplace(false);
	Registry& bvhReg = mBVHWorld->GetRegistry();

	for (uint32 i = 0; i < mPolygonDataNavMesh.size(); i++)
	{
		const entt::entity entity = bvhReg.Create(entt::entity{ i });
		ASSERT(entity == entt::entity{ i });
		bvhReg.AddComponent<PhysicsBody2DComponent>(entity).mRules = CollisionPresets::sTerrain.mRules;
		bvhReg.AddComponent<TransformedPolygonColliderComponent>(entity, mPolygonDataNavMesh[i]);
	}

	BVH& bvh = mBVHWorld->GetPhysics().GetBVHs()[static_cast<int>(CollisionPresets::sTerrain.mRules.mLayer)];
	bvh.Build();
}

CE::NavMeshComponent::NavMeshData CE::NavMeshComponent::GenerateNavMeshData(const World& world) const
{
	NavMeshData data{};

	{ // Generate obstacles
		const auto& polygonView = world.GetRegistry().View<TransformedPolygonColliderComponent, PhysicsBody2DComponent>();
		for (const auto [entity, polygonCollider, body] : polygonView.each())
		{
			std::vector<TransformedPolygon>* partOfList{};

			if (body.mRules.mLayer == CollisionLayer::StaticObstacles)
			{
				partOfList = &data.mObstacles;
			}
			else if (body.mRules.mLayer == CollisionLayer::Terrain)
			{
				partOfList = &data.mWalkable;
			}
			else
			{
				continue;
			}

			partOfList->emplace_back(polygonCollider);
		}

		const auto& diskView = world.GetRegistry().View<TransformedDiskColliderComponent, PhysicsBody2DComponent>();
		for (const auto [entity, diskCollider, body] : diskView.each())
		{
			std::vector<TransformedPolygon>* partOfList{};

			if (body.mRules.mLayer == CollisionLayer::StaticObstacles)
			{
				partOfList = &data.mObstacles;
			}
			else if (body.mRules.mLayer == CollisionLayer::Terrain)
			{
				partOfList = &data.mWalkable;
			}
			else
			{
				continue;
			}

			partOfList->emplace_back(diskCollider.GetAsPolygon());
		}

		const auto& aabbView = world.GetRegistry().View<TransformedAABBColliderComponent, PhysicsBody2DComponent>();
		for (const auto [entity, aabbCollider, body] : aabbView.each())
		{
			std::vector<TransformedPolygon>* partOfList{};

			if (body.mRules.mLayer == CollisionLayer::StaticObstacles)
			{
				partOfList = &data.mObstacles;
			}
			else if (body.mRules.mLayer == CollisionLayer::Terrain)
			{
				partOfList = &data.mWalkable;
			}
			else
			{
				continue;
			}

			partOfList->emplace_back(aabbCollider.GetAsPolygon());
		}
	}

	return data;
}

std::vector<CE::TransformedPolygon> CE::NavMeshComponent::GetDifferences(const NavMeshData& navMeshData)
{
	std::vector<TransformedPolygon> differences;

	// Initialize Clipper2Lib data structures for polygon operations
	Clipper2Lib::PathD doublePolygon;
	Clipper2Lib::PathsD walkableUnion;

	// Process walkable polygons
	for (const auto& polygonListElement : navMeshData.mWalkable)
	{
		for (const auto& polygonElementVertex : polygonListElement.mPoints)
		{
			doublePolygon.push_back(Clipper2Lib::PointD(polygonElementVertex.x, polygonElementVertex.y));
		}
		walkableUnion.push_back(doublePolygon);
		doublePolygon.clear();
	}
	walkableUnion = Union(walkableUnion, Clipper2Lib::FillRule::NonZero);

	// Initialize Clipper2Lib data structures for obstacle polygons
	Clipper2Lib::PathsD obstacleUnion;

	// Process obstacle polygons
	for (const auto& polygonListElement : navMeshData.mObstacles)
	{
		doublePolygon.clear();
		for (const auto& polygonElementVertex : polygonListElement.mPoints)
		{
			doublePolygon.push_back(Clipper2Lib::PointD(polygonElementVertex.x, polygonElementVertex.y));
		}
		obstacleUnion.push_back(doublePolygon);
	}
	obstacleUnion = Union(obstacleUnion, Clipper2Lib::FillRule::NonZero);

	// Calculate the difference between walkable and obstacle polygons
	const Clipper2Lib::PathsD& remainingDifference = Difference(walkableUnion, obstacleUnion,
	                                                            Clipper2Lib::FillRule::NonZero, 2);

	// Process remaining polygons after cleanup
	for (const auto& polygonListElement : remainingDifference)
	{
		PolygonPoints floatPolygon{};
		floatPolygon.reserve(polygonListElement.size());

		for (const auto& polygonElementVertex : polygonListElement)
		{
			glm::vec2 vertex = {polygonElementVertex.x, polygonElementVertex.y};
			floatPolygon.push_back(vertex);
		}

		differences.emplace_back(std::move(floatPolygon));
	}

	return differences;
}

void CE::NavMeshComponent::Triangulation(const std::vector<TransformedPolygon>& polygonList)
{
	// Initialize a constrained Delaunay triangulation (CDT) object
	CDT::Triangulation<float> cdt;

	// Create a container for all vertices in the polygonList
	Clipper2Lib::PathD allVertices;

	// Convert and insert vertices into the CDT
	// (If the insert vertices looks similar to Valentina's, it's cause I kinda took the conversion from vec2 to PointD code from her
	// since it was the only thing that seemed to fix the issue of not being able to triangulate map 3 and 5.)
	for (const auto& polygon : polygonList)
	{
		for (const auto& point : polygon.mPoints)
		{
			Clipper2Lib::PointD pointD = {point.x, point.y};
			allVertices.emplace_back(pointD);
		}
	}

	cdt.insertVertices(allVertices.begin(), allVertices.end(),
	                   [](const Clipper2Lib::PointD& p) { return p.x; },
	                   [](const Clipper2Lib::PointD& p) { return p.y; });

	// Create constraint edges based on polygon vertices
	std::vector<CDT::Edge> constraintEdge;
	int i = 0;
	for (auto& polygon : polygonList)
	{
		for (int j = 0; j < static_cast<int>(polygon.mPoints.size()); j++)
		{
			constraintEdge.push_back({
				CDT::Edge(static_cast<CDT::VertInd>(i + j), static_cast<CDT::VertInd>(i + (j + 1) % polygon.mPoints.size()))
			});
		}
		i += static_cast<int>(polygon.mPoints.size());
	}

	// Insert constraint edges into the CDT
	cdt.insertEdges(constraintEdge);
	// Erase outer triangles and holes to clean up the triangulation
	cdt.eraseOuterTrianglesAndHoles();

	// Extract triangles from the CDT and add them to PolygonDataNavMesh
	for (const auto& [vertices, neighbors] : cdt.triangles)
	{
		const TransformedPolygon& polygon = mPolygonDataNavMesh.emplace_back(
			PolygonPoints{
				{cdt.vertices[vertices[0]].x, cdt.vertices[vertices[0]].y},
				{cdt.vertices[vertices[1]].x, cdt.vertices[vertices[1]].y},
				{cdt.vertices[vertices[2]].x, cdt.vertices[vertices[2]].y}
			});

		// Calculate the center of the triangle and add it as a node to AStarGraph
		const glm::vec2 centerOfTriangle = polygon.GetCentre();
		mAStarGraph.AddNode(centerOfTriangle.x, centerOfTriangle.y);
	}

	// Create edges between nodes based on triangle neighbors
	for (int k = 0; k < static_cast<int>(cdt.triangles.size()); k++)
	{
		for (int j = 0; j < 3; j++)
		{
			if (cdt.triangles[k].neighbors[j] < cdt.triangles.size())
			{
				mAStarGraph.mNodes[k].AddEdge(&mAStarGraph.mNodes[cdt.triangles[k].neighbors[j]]);
			}
		}
	}
}

std::vector<glm::vec2> CE::NavMeshComponent::FunnelAlgorithm(const std::vector<PolygonPoints>& triangles,
                                                         glm::vec2 start, glm::vec2 goal) const
{
	std::vector<glm::vec2> path{};
	std::vector<glm::vec2> funnelRight{};
	std::vector<glm::vec2> funnelLeft{};

	// Check if there are enough triangles to form a path
	if (triangles.size() < 3)
	{
		return {};
	}

	// Initialize the path with the start position
	path.push_back(start);

	// Compute overlapping vertices between the first two triangles
	std::vector<glm::vec2> overlappingVertexes;
	for (auto currentTriangleVertex : triangles[0])
	{
		for (auto nextTriangleVertex : triangles[1])
		{
			if (currentTriangleVertex == nextTriangleVertex)
			{
				if (!overlappingVertexes.empty())
				{
					if (overlappingVertexes[0] != currentTriangleVertex)
					{
						overlappingVertexes.push_back(currentTriangleVertex);
						break;
					}
				}
				else
				{
					overlappingVertexes.push_back(currentTriangleVertex);
					break;
				}
			}
		}
	}

	// Calculate the middle point between overlapping vertices
	const glm::vec2 middlePoint = (overlappingVertexes[1] + overlappingVertexes[0]) / 2.0f;

	// Determine the initial funnel edges of each side based on start position and overlapping vertices    
	if (IsPointLeftOfLine(overlappingVertexes[0], start, middlePoint))
	{
		funnelLeft.push_back(overlappingVertexes[0]);
		funnelRight.push_back(overlappingVertexes[1]);
	}
	else
	{
		funnelLeft.push_back(overlappingVertexes[1]);
		funnelRight.push_back(overlappingVertexes[0]);
	}

	// Iterate through the remaining triangles
	for (uint32 i = 2; i < triangles.size(); i++)
	{
		// Find overlapping vertices between current triangle and previous triangle
		overlappingVertexes.clear();
		for (const auto& currentTriangleVertex : triangles[i - 1])
		{
			for (const auto& nextTriangleVertex : triangles[i])
			{
				if (currentTriangleVertex != nextTriangleVertex)
				{
					continue;
				}

				if (!overlappingVertexes.empty())
				{
					if (overlappingVertexes[0] != currentTriangleVertex)
					{
						overlappingVertexes.push_back(currentTriangleVertex);
						break;
					}
				}
				else
				{
					overlappingVertexes.push_back(currentTriangleVertex);
					break;
				}
			}
		}

		bool goesIntoTheLeft = false;
		glm::vec2 vertexToCheck{};

		// Determine if the path goes into the left or right funnel
		for (uint32 j = 0; j < overlappingVertexes.size(); j++)
		{
			if (funnelLeft.back() == overlappingVertexes[j])
			{
				goesIntoTheLeft = false;
				vertexToCheck = overlappingVertexes[(j + 1) % 2];
			}
			else if (funnelRight.back() == overlappingVertexes[j])
			{
				goesIntoTheLeft = true;
				vertexToCheck = overlappingVertexes[(j + 1) % 2];
			}
		}

		// Adjust the funnel edges based on the new vertex
		if (goesIntoTheLeft)
		{
			// Adjust the left funnel edge
			while (funnelLeft.size() > 1 && IsPointRightOfLine(
				vertexToCheck, path.back(), funnelLeft[funnelLeft.size() - 2]))
			{
				funnelLeft.pop_back();
			}

			funnelLeft.push_back(vertexToCheck);

			while (IsPointLeftOfLine(funnelRight.front(), path.back(), funnelLeft.back()) &&
				funnelRight.size() > 1)
			{
				path.push_back(funnelRight.front());
				funnelRight.erase(funnelRight.begin());
			}
		}
		else
		{
			// Adjust the right funnel edge
			while (funnelRight.size() > 1 && IsPointLeftOfLine(
				vertexToCheck, path.back(), funnelRight[funnelRight.size() - 2]))
			{
				funnelRight.pop_back();
			}

			funnelRight.push_back(vertexToCheck);

			while (IsPointRightOfLine(funnelLeft.front(), path.back(), funnelRight.back()) &&
				funnelLeft.size() > 1)
			{
				path.push_back(funnelLeft.front());
				funnelLeft.erase(funnelLeft.begin());
			}
		}
	}

	// // Determine the shortest path between the funnel edges and the goal and add points accordingly
	if (funnelLeft.size() <= funnelRight.size())
	{
		for (uint32 i = 0; i < funnelLeft.size(); i++)
		{
			if (distance(funnelLeft[i], goal) <= distance(funnelRight[i], goal))
			{
				path.push_back(funnelLeft[i]);
			}
			else
			{
				path.push_back(funnelRight[i]);
			}
		}
	}
	else
	{
		for (uint32 i = 0; i < funnelRight.size(); i++)
		{
			if (distance(funnelRight[i], goal) <= distance(funnelLeft[i], goal))
			{
				path.push_back(funnelRight[i]);
			}
			else
			{
				path.push_back(funnelLeft[i]);
			}
		}
	}

	return path;
}

void CE::NavMeshComponent::UpdateNavMesh()
{
	mNavMeshNeedsUpdate = true;
}

std::vector<glm::vec2> CE::NavMeshComponent::CleanupPathfinding(const std::vector<const Pathfinding::Node*>& nodes,
                                                            glm::vec2 start, glm::vec2 goal) const
{
	std::vector<glm::vec2> path{};

	path.reserve(nodes.size() + 2);

	// Initialize the path with the start position
	path.emplace_back(start);

	// Check if there are enough nodes to form a path
	if (nodes.size() < 3)
	{
		path.emplace_back(goal);
		return path;
	}

	for (uint32 i = 1; i < nodes.size() - 1; i++)
	{
		std::array<glm::vec2, 2> line{};
		uint32 numPointsFound{};

		for (const glm::vec2& currentTriangleVertex : mPolygonDataNavMesh[nodes[i]->GetId()].mPoints)
		{
			for (const glm::vec2& nextTriangleVertex : mPolygonDataNavMesh[nodes[i + 1]->GetId()].mPoints)
			{
				if (currentTriangleVertex != nextTriangleVertex)
				{
					continue;
				}

				line[numPointsFound++] = currentTriangleVertex;

				if (numPointsFound != 2)
				{
					continue;
				}

				const glm::vec2 middlePoint = (line[0] + line[1]) * .5f;
				path.emplace_back(middlePoint);
				goto next;
			}
		}
	next:;
	}

	path.emplace_back(goal);
	return path;
}

CE::MetaType CE::NavMeshComponent::Reflect()
{
	auto type = MetaType{MetaType::T<NavMeshComponent>{}, "NavMeshComponent"};
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);

	type.AddFunc(&NavMeshComponent::UpdateNavMesh, "UpdateNavMesh").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sCallFromEditorTag);
	ReflectComponentType<NavMeshComponent>(type);
	return type;
}

std::vector<glm::vec2> CE::NavMeshComponent::FindQuickestPath(glm::vec2 startPos, glm::vec2 endPos) const
{
	std::vector<glm::vec2> pathFound{};

	if (!mBVHWorld.has_value())
	{
		LOG(LogWorld, Error, "Could not find path, navmesh was not build yet");
		return pathFound;
	}

	// Initialize pointers to the start and end nodes
	entt::entity startNodeOwner = entt::null;
	entt::entity endNodeOwner = entt::null;

	const BVH& bvh = mBVHWorld->GetPhysics().GetBVHs()[static_cast<int>(CollisionPresets::sTerrain.mRules.mLayer)];

	struct OnIntersect
	{
		static void Callback(const TransformedPolygonColliderComponent&, entt::entity entity, entt::entity& out)
		{
			out = entity;
		}

		static void Callback(const TransformedDiskColliderComponent&, entt::entity, entt::entity&) {}
		static void Callback(const TransformedAABBColliderComponent&, entt::entity, entt::entity&) {}
	};

	bvh.Query<OnIntersect, BVH::DefaultShouldReturnFunction<true>, BVH::DefaultShouldReturnFunction<true>>(startPos, startNodeOwner);
	bvh.Query<OnIntersect, BVH::DefaultShouldReturnFunction<true>, BVH::DefaultShouldReturnFunction<true>>(endPos, endNodeOwner);


	// Check if either the start or end node is not found
	if (startNodeOwner == entt::null
		|| endNodeOwner == entt::null)
	{
		return pathFound;
	}

	const Pathfinding::Node& start = mAStarGraph.mNodes[static_cast<int>(startNodeOwner)];
	const Pathfinding::Node& end = mAStarGraph.mNodes[static_cast<int>(endNodeOwner)];

	// Initialize a vector to store the node path found by the A* algorithm
	const std::vector<const Pathfinding::Node*> nodePathFound = mAStarGraph.AStarSearch(&start, &end);
	pathFound = CleanupPathfinding(nodePathFound, startPos, endPos);

	return pathFound;
}

void CE::NavMeshComponent::DebugDrawNavMesh(const World& world) const
{
	if (!DebugRenderer::IsCategoryVisible(DebugCategory::AINavigation))
	{
		return;
	}

	const auto& view = world.GetRegistry().View<NavMeshComponent>();
	if (view.empty())
	{
		return;
	}

	for (auto& agentId : view)
	{
		auto [navMesh] = view.get(agentId);

		auto cleanedPolygonList = navMesh.GetCleanedPolygonList();

		for (int h = 0; h < static_cast<int>(cleanedPolygonList.size()); h++)
		{
			// Choose a color for the polygon, green for the first one and red for others
			const glm::vec4 colour = {1.f, (h == 0 ? 1.f : 0.f), 0.f, 1.f};

			for (int j = 0; j < static_cast<int>(cleanedPolygonList[h].mPoints.size()); j++)
			{
				if (j + 1 == static_cast<int>(cleanedPolygonList[h].mPoints.size()))
				{
					// Draw a line connecting the last vertex to the first vertex
					DrawDebugLine(world, DebugCategory::AINavigation,
						{ cleanedPolygonList[h].mPoints[j].x, 0, cleanedPolygonList[h].mPoints[j].y },
						{ cleanedPolygonList[h].mPoints[0].x, 0, cleanedPolygonList[h].mPoints[0].y },
						colour);
				}
				else
				{
					// Draw a line connecting two consecutive vertices
					DrawDebugLine(world, DebugCategory::AINavigation,
						{ cleanedPolygonList[h].mPoints[j].x, 0, cleanedPolygonList[h].mPoints[j].y },
													 {
														 cleanedPolygonList[h].mPoints[j + 1].x, 0,
														 cleanedPolygonList[h].mPoints[j + 1].y
													 },
						colour);
				}
			}
		}

		auto polygonDataNavMesh = navMesh.GetPolygonDataNavMesh();
		//explained and inspired by Ayoub's version though it has been heavily changed but the idea still remains.
		for (const auto& polygon : polygonDataNavMesh)
		{
			const PolygonPoints& points = polygon.mPoints;

			// Draw the edges of each triangle with a blue color
			DrawDebugLine(world, DebugCategory::AINavigation, { points[0].x, 0, points[0].y }, { points[1].x, 0, points[1].y }, {0.f, 0.f, 1.f, 1.f});

			DrawDebugLine(world, DebugCategory::AINavigation, { points[1].x, 0, points[1].y }, { points[2].x, 0, points[2].y },
				{0.f, 0.f, 1.f, 1.f});

			DrawDebugLine(world, DebugCategory::AINavigation, { points[2].x, 0, points[2].y }, { points[0].x, 0, points[0].y }, glm::vec4{0.f, 0.f, 1.f, 1.f});
		}
	}

	mAStarGraph.DebugDrawAStarGraph(world);
}
