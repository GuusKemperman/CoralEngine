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
#include "Components/TransformComponent.h"
#include "Components/Pathfinding/NavMeshObstacleTag.h"
#include "Components/Physics2D/DiskColliderComponent.h"
#include "Components/Physics2D/PolygonColliderComponent.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"
#include "World/Registry.h"
#include "World/World.h"
#include "World/WorldRenderer.h"

using namespace Engine;

NavMeshComponent::NavMeshComponent()
{
}

void NavMeshComponent::SetNavMesh(const World& world)
{
	mPolygonDataNavMesh = {};
	mCleanedPolygonList = {};
	mAStarGraph.ListOfNodes = {};

	CleanupGeometry(LoadNavMeshData(world));
	Triangulation(mCleanedPolygonList);
	mNavMeshNeedsUpdate = false;
}

std::vector<geometry2d::PolygonList> NavMeshComponent::LoadNavMeshData(const World& world) const
{
	//// initialize a vector to store the messy polygons
	std::vector<geometry2d::PolygonList> messypolygons;

	// initialize lists to store walkable and obstacle polygons
	geometry2d::PolygonList walkablelist = {};
	geometry2d::PolygonList obstaclelist = {};

	const auto& polygonView = world.GetRegistry().View<
		PolygonColliderComponent, PhysicsBody2DComponent, TransformComponent, NavMeshObstacleTag>();
	const auto& diskView = world.GetRegistry().View<DiskColliderComponent, TransformComponent, NavMeshObstacleTag>();

	for (const auto& polygonId : polygonView)
	{
		const auto& [polygonCollider, rigidBody, transform] = polygonView.get(polygonId);
		geometry2d::Polygon polygon;

		const glm::vec2 pos = transform.GetWorldPosition2D();
		for (const auto& coordinate : polygonCollider.mPoints)
		{
			polygon.push_back(coordinate + pos);
		}

		obstaclelist.push_back(polygon);
	}
	for (const auto& diskId : diskView)
	{
		const auto& [diskCollider, transform] = diskView.get(diskId);
		geometry2d::Polygon polygon;

		const glm::vec2 pos = transform.GetWorldPosition2D();

		polygon.emplace_back(diskCollider.mRadius + pos.x, pos.y);
		polygon.emplace_back(pos.x, diskCollider.mRadius + pos.y);
		polygon.emplace_back(-diskCollider.mRadius + pos.x, pos.y);
		polygon.emplace_back(pos.x, -diskCollider.mRadius + pos.y);

		obstaclelist.push_back(polygon);
	}

	walkablelist.push_back(mBorderCorners);

	// add the walkable and obstacle lists to the messy polygons vector
	messypolygons.push_back(walkablelist);
	messypolygons.push_back(obstaclelist);

	return messypolygons;
}

void NavMeshComponent::CleanupGeometry(const std::vector<geometry2d::PolygonList>& dirtyPolygonList)
{
	// Initialize temporary variables
	geometry2d::PolygonList tempPolygonList;

	// Extract walkable and obstacle polygons from the input
	const geometry2d::PolygonList& walkables = dirtyPolygonList[0];
	const geometry2d::PolygonList& obstacles = dirtyPolygonList[1];

	// Initialize Clipper2Lib data structures for polygon operations
	Clipper2Lib::PathD doublePolygon;
	Clipper2Lib::PathsD walkableUnion;

	// Process walkable polygons
	for (const auto& polygonListElement : walkables)
	{
		for (const auto& polygonElementVertex : polygonListElement)
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
	for (const auto& polygonListElement : obstacles)
	{
		doublePolygon.clear();
		for (const auto& polygonElementVertex : polygonListElement)
		{
			doublePolygon.push_back(Clipper2Lib::PointD(polygonElementVertex.x, polygonElementVertex.y));
		}
		obstacleUnion.push_back(doublePolygon);
	}
	obstacleUnion = Union(obstacleUnion, Clipper2Lib::FillRule::NonZero);

	// Calculate the difference between walkable and obstacle polygons
	const Clipper2Lib::PathsD& remainingDifference = Difference(walkableUnion, obstacleUnion,
	                                                            Clipper2Lib::FillRule::NonZero, 2);

	// Initialize a temporary polygon for conversion
	geometry2d::Polygon floatPolygon;

	// Process remaining polygons after cleanup
	for (const auto& polygonListElement : remainingDifference)
	{
		floatPolygon.clear();
		for (const auto& polygonElementVertex : polygonListElement)
		{
			glm::vec2 vertex = {polygonElementVertex.x, polygonElementVertex.y};
			floatPolygon.push_back(vertex);
		}
		tempPolygonList.push_back(floatPolygon);
	}

	// Clear and update the cleaned polygon list
	mCleanedPolygonList.clear();
	mCleanedPolygonList = tempPolygonList;
}

void NavMeshComponent::Triangulation(const geometry2d::PolygonList& polygonList)
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
		for (const auto& point : polygon)
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
		for (int j = 0; j < static_cast<int>(polygon.size()); j++)
		{
			constraintEdge.push_back({
				CDT::Edge(static_cast<CDT::VertInd>(i + j), static_cast<CDT::VertInd>(i + (j + 1) % polygon.size()))
			});
		}
		i += static_cast<int>(polygon.size());
	}

	// Insert constraint edges into the CDT
	cdt.insertEdges(constraintEdge);
	// Erase outer triangles and holes to clean up the triangulation
	cdt.eraseOuterTrianglesAndHoles();

	// Extract triangles from the CDT and add them to PolygonDataNavMesh
	for (const auto& [vertices, neighbors] : cdt.triangles)
	{
		geometry2d::Polygon polygon = {
			{cdt.vertices[vertices[0]].x, cdt.vertices[vertices[0]].y},
			{cdt.vertices[vertices[1]].x, cdt.vertices[vertices[1]].y},
			{cdt.vertices[vertices[2]].x, cdt.vertices[vertices[2]].y}
		};
		mPolygonDataNavMesh.push_back(polygon);

		// Calculate the center of the triangle and add it as a node to AStarGraph
		const glm::vec2 centerOfTriangle = geometry2d::ComputeCenterOfPolygon(polygon);
		mAStarGraph.AddNode(centerOfTriangle.x, centerOfTriangle.y);
	}

	// Create edges between nodes based on triangle neighbors
	for (int k = 0; k < static_cast<int>(cdt.triangles.size()); k++)
	{
		for (int j = 0; j < 3; j++)
		{
			if (cdt.triangles[k].neighbors[j] < cdt.triangles.size())
			{
				mAStarGraph.ListOfNodes[k].AddEdge(&mAStarGraph.ListOfNodes[cdt.triangles[k].neighbors[j]]);
			}
		}
	}
}

std::vector<glm::vec2> NavMeshComponent::FunnelAlgorithm(const std::vector<geometry2d::Polygon>& triangles,
                                                         const glm::vec2& start, const glm::vec2& goal) const
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
	if (geometry2d::IsPointLeftOfLine(overlappingVertexes[0], start, middlePoint))
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

		bool goesIntoTheLeft = false;
		glm::vec2 vertexToCheck;

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
			while (funnelLeft.size() > 1 && geometry2d::IsPointRightOfLine(
				vertexToCheck, path.back(), funnelLeft[funnelLeft.size() - 2]))
			{
				funnelLeft.pop_back();
			}

			funnelLeft.push_back(vertexToCheck);

			while (geometry2d::IsPointLeftOfLine(funnelRight.front(), path.back(), funnelLeft.back()) &&
				funnelRight.size() > 1)
			{
				path.push_back(funnelRight.front());
				funnelRight.erase(funnelRight.begin());
			}
		}
		else
		{
			// Adjust the right funnel edge
			while (funnelRight.size() > 1 && geometry2d::IsPointLeftOfLine(
				vertexToCheck, path.back(), funnelRight[funnelRight.size() - 2]))
			{
				funnelRight.pop_back();
			}

			funnelRight.push_back(vertexToCheck);

			while (geometry2d::IsPointRightOfLine(funnelLeft.front(), path.back(), funnelRight.back()) &&
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

void NavMeshComponent::UpdateNavMesh()
{
	mNavMeshNeedsUpdate = true;
}

MetaType NavMeshComponent::Reflect()
{
	auto type = MetaType{MetaType::T<NavMeshComponent>{}, "NavMeshComponent"};
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	type.AddField(&NavMeshComponent::mBorderCorners, "BorderCorners").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&NavMeshComponent::UpdateNavMesh, "UpdateSquare", "").GetProperties().Add(Props::sIsScriptableTag).Add(
		Props::sCallFromEditorTag);
	ReflectComponentType<NavMeshComponent>(type);
	return type;
}

std::vector<glm::vec2> NavMeshComponent::FindQuickestPath(const glm::vec2& startPos, const glm::vec2& endPos) const
{
	// Initialize pointers to the start and end nodes
	const Node* startNode = nullptr;
	const Node* endNode = nullptr;

	// Find the start and end nodes based on their positions
	for (int i = 0; i < static_cast<int>(mPolygonDataNavMesh.size()); i++)
	{
		if (geometry2d::IsPointInsidePolygon({startPos[0], startPos[1]}, mPolygonDataNavMesh[i]))
		{
			startNode = &mAStarGraph.ListOfNodes[i];
		}
		if (geometry2d::IsPointInsidePolygon({endPos[0], endPos[1]}, mPolygonDataNavMesh[i]))
		{
			endNode = &mAStarGraph.ListOfNodes[i];
		}
		if (startNode != nullptr && endNode != nullptr)
		{
			break;
		}
	}

	// Check if either the start or end node is not found
	if (startNode == nullptr || endNode == nullptr)
	{
		std::vector<glm::vec2> emptyList{};
		emptyList.clear();
		return emptyList;
	}

	// Initialize a vector to store the node path found by the A* algorithm
	std::vector<const Node*> nodePathFound;
	std::vector<geometry2d::Polygon> trianglePathFound; // This stores the polygons corresponding to the node path

	// Perform A* search if start and end nodes are different
	if (startNode != endNode)
	{
		nodePathFound = mAStarGraph.AStarSearch(startNode, endNode);

		// Convert nodes to corresponding polygons
		for (const auto& node : nodePathFound)
		{
			trianglePathFound.push_back(mPolygonDataNavMesh[node->GetId()]);
		}
	}
	else
	{
		nodePathFound = {};
	}

	std::vector<glm::vec2> pathFound = {};

	// Compute the shortest path with the funnel algorithm between start and end positions
	pathFound = FunnelAlgorithm(trianglePathFound, startPos, endPos);

	//pathFound.reserve(nodePathFound.size());

	// // Convert polygons to glm::vec2 positions (centers of polygons)
	// for (const auto& triangle : trianglePathFound)
	// {
	//     pathFound.push_back(bee::geometry2d::ComputeCenterOfPolygon(triangle));
	// }

	// Modify the pathFound vector as needed
	if (pathFound.empty())
	{
		pathFound = {startPos};
	}
	else
	{
		pathFound[1] = {startPos};
		pathFound.erase(pathFound.begin());
	}

	// if (pathFound.size() > 1)
	// {
	//     pathFound[pathFound.size()-1] = {endPos};
	// }
	// else
	// {
	pathFound.push_back({endPos});
	//}

	return pathFound;
}

geometry2d::PolygonList NavMeshComponent::GetCleanedPolygonList() const
{
	return mCleanedPolygonList;
}

geometry2d::PolygonList NavMeshComponent::GetPolygonDataNavMesh() const
{
	return mPolygonDataNavMesh;
}

void NavMeshComponent::DebugDrawNavMesh(const World& world) const
{
	const auto& view = world.GetRegistry().View<NavMeshComponent>();
	if (view.empty()) { return; }

	//const auto* transformView = world.GetRegistry().TryGet<TransformComponent>(view.front());

	for (auto& agentId : view)
	{
		auto [navMesh] = view.get(agentId);

		auto cleanedPolygonList = navMesh.GetCleanedPolygonList();

		std::vector<glm::vec3> renderBorder;
		for (uint32 i = 0; i < mBorderCorners.size(); i++)
		{
			renderBorder.push_back({mBorderCorners[i].x, 0, mBorderCorners[i].y});
		}

		world.GetRenderer().AddPolygon(DebugCategory::AINavigation, renderBorder, {1.f, 0.f, 0.f, 1.f});

		for (int h = 0; h < static_cast<int>(cleanedPolygonList.size()); h++)
		{
			// Choose a color for the polygon, green for the first one and red for others
			const glm::vec4 colour = {1.f, (h == 0 ? 1.f : 0.f), 0.f, 1.f};

			for (int j = 0; j < static_cast<int>(cleanedPolygonList[h].size()); j++)
			{
				if (j + 1 == static_cast<int>(cleanedPolygonList[h].size()))
				{
					// Draw a line connecting the last vertex to the first vertex


					world.GetRenderer().AddLine(DebugCategory::Gameplay,
					                            {cleanedPolygonList[h][j].x, 0, cleanedPolygonList[h][j].y},
					                            {cleanedPolygonList[h][0].x, 0, cleanedPolygonList[h][0].y}, colour);
				}
				else
				{
					// Draw a line connecting two consecutive vertices
					world.GetRenderer().AddLine(DebugCategory::Gameplay,
					                            {cleanedPolygonList[h][j].x, 0, cleanedPolygonList[h][j].y},
					                            {cleanedPolygonList[h][j + 1].x, 0, cleanedPolygonList[h][j + 1].y},
					                            colour);
				}
			}
		}

		auto polygonDataNavMesh = navMesh.GetPolygonDataNavMesh();
		//explained and inspired by Ayoub's version though it has been heavily changed but the idea still remains.
		for (const auto& polygonList : polygonDataNavMesh)
		{
			// Draw the edges of each triangle with a blue color
			world.GetRenderer().AddLine(DebugCategory::Gameplay, {polygonList[0].x, 0, polygonList[0].y},
			                            {polygonList[1].x, 0, polygonList[1].y},
			                            {0.f, 0.f, 1.f, 1.f});
			world.GetRenderer().AddLine(DebugCategory::Gameplay, {polygonList[1].x, 0, polygonList[1].y},
			                            {polygonList[2].x, 0, polygonList[2].y},
			                            {0.f, 0.f, 1.f, 1.f});
			world.GetRenderer().AddLine(DebugCategory::Gameplay, {polygonList[2].x, 0, polygonList[2].y},
			                            {polygonList[0].x, 0, polygonList[0].y},
			                            {0.f, 0.f, 1.f, 1.f});
		}
	}

	mAStarGraph.DebugDrawAStarGraph(world);
}
