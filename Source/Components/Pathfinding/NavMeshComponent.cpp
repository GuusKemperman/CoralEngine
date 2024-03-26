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
#include "Components/Physics2D/DiskColliderComponent.h"
#include "Components/Physics2D/PolygonColliderComponent.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"
#include "World/Registry.h"
#include "World/World.h"
#include "Utilities/DrawDebugHelpers.h"

void Engine::NavMeshComponent::GenerateNavMesh(const World& world)
{
	mPolygonDataNavMesh.clear();
	mCleanedPolygonList.clear();
	mAStarGraph = {};

	NavMeshData navMeshData = GenerateNavMeshData(world);
	mCleanedPolygonList = GetDifferences(navMeshData.mWalkable, navMeshData.mObstacles);
	Triangulation(mCleanedPolygonList);
	mNavMeshNeedsUpdate = false;
}

Engine::NavMeshComponent::NavMeshData Engine::NavMeshComponent::GenerateNavMeshData(const World& world) const
{
	NavMeshData data{};

	glm::vec2 terrainStart{};

	{ // Generate obstacles
		const auto& polygonView = world.GetRegistry().View<PolygonColliderComponent, PhysicsBody2DComponent, TransformComponent>();
		for (const auto [entity, polygonCollider, body, transform] : polygonView.each())
		{
			PolygonList* partOfList{};
			const glm::vec2 pos = transform.GetWorldPosition2D();

			if (body.mRules.mLayer == CollisionLayer::StaticObstacles)
			{
				partOfList = &data.mObstacles;
			}
			else if (body.mRules.mLayer == CollisionLayer::Terrain)
			{
				partOfList = &data.mWalkable;
				terrainStart = pos;
			}
			else
			{
				continue;
			}

			Polygon polygon;

			for (const auto& coordinate : polygonCollider.mPoints)
			{
				polygon.push_back(coordinate + pos);
			}

			partOfList->emplace_back(std::move(polygon));
		}

		const auto& diskView = world.GetRegistry().View<DiskColliderComponent, PhysicsBody2DComponent, TransformComponent>();
		for (const auto [entity, diskCollider, body, transform] : diskView.each())
		{
			PolygonList* partOfList{};
			const glm::vec2 center = transform.GetWorldPosition2D();

			if (body.mRules.mLayer == CollisionLayer::StaticObstacles)
			{
				partOfList = &data.mObstacles;
			}
			else if (body.mRules.mLayer == CollisionLayer::Terrain)
			{
				partOfList = &data.mWalkable;
				terrainStart = center;
			}
			else
			{
				continue;
			}

			Polygon polygon;

			constexpr float dt = glm::two_pi<float>() / 16.0f;

			for (float t = 0.0f; t < glm::two_pi<float>() - dt; t += dt)
			{
				polygon.emplace_back(center.x + diskCollider.mRadius * cos(t + dt), center.y + diskCollider.mRadius * sin(t + dt));
			}

			partOfList->emplace_back(std::move(polygon));
		}
	}


	{ // Artificially place obstacles based on height differences
		std::queue<glm::vec2> open{};
		open.emplace(terrainStart);

		std::vector<glm::vec2> closed{};
		closed.reserve(2048);

		const Physics& physics = world.GetPhysics();

		static constexpr float spacing = 1.0f;
		while (!open.empty())
		{
			glm::vec2 current = open.front();
			open.pop();

			if (std::find(closed.begin(), closed.end(), current) != closed.end())
			{
				continue;
			}
			closed.emplace_back(current);

			const float heightAtCurrent = physics.GetHeightAtPosition(current);

			if (heightAtCurrent == -std::numeric_limits<float>::infinity())
			{
				continue;
			}

			auto processNbr = [&](const glm::vec2 nbr)
				{
					const float heightAtNeighbour = physics.GetHeightAtPosition(nbr);

					if (heightAtNeighbour == -std::numeric_limits<float>::infinity())
					{
						return;
					}

					open.emplace(nbr);

					if (Physics::IsHeightDifferenceTraversable(heightAtCurrent - heightAtNeighbour))
					{
						return;
					}

					const glm::vec2 halfToNbr = (nbr - current) * .5f;
					const glm::vec2 perpToNbr = glm::vec2{ halfToNbr.y, -halfToNbr.x };
					const glm::vec2 obstacleStart = current + halfToNbr - perpToNbr;
					const glm::vec2 obstacleEnd = current + halfToNbr + perpToNbr;
					const glm::vec2 wiggleRoom = halfToNbr * 0.01f;

					// Otherwise, we place an obstacle.
					data.mObstacles.emplace_back(Polygon{ obstacleStart + wiggleRoom, obstacleEnd + wiggleRoom, obstacleEnd - wiggleRoom, obstacleStart - wiggleRoom });
				};

			processNbr({ current.x - spacing, current.y });
			processNbr({ current.x + spacing, current.y });
			processNbr({ current.x, current.y - spacing });
			processNbr({ current.x, current.y + spacing });
		}
	}

	return data;
}

Engine::PolygonList Engine::NavMeshComponent::GetDifferences(const PolygonList& walkables, const PolygonList& obstacles)
{
	PolygonList differences;

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
	Polygon floatPolygon;

	// Process remaining polygons after cleanup
	for (const auto& polygonListElement : remainingDifference)
	{
		floatPolygon.clear();
		for (const auto& polygonElementVertex : polygonListElement)
		{
			glm::vec2 vertex = {polygonElementVertex.x, polygonElementVertex.y};
			floatPolygon.push_back(vertex);
		}
		differences.push_back(floatPolygon);
	}

	return differences;
}

void Engine::NavMeshComponent::Triangulation(const Engine::PolygonList& polygonList)
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
		Polygon polygon = {
			{cdt.vertices[vertices[0]].x, cdt.vertices[vertices[0]].y},
			{cdt.vertices[vertices[1]].x, cdt.vertices[vertices[1]].y},
			{cdt.vertices[vertices[2]].x, cdt.vertices[vertices[2]].y}
		};
		mPolygonDataNavMesh.push_back(polygon);

		// Calculate the center of the triangle and add it as a node to AStarGraph
		const glm::vec2 centerOfTriangle = ComputeCenterOfPolygon(polygon);
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

std::vector<glm::vec2> Engine::NavMeshComponent::FunnelAlgorithm(const std::vector<Polygon>& triangles,
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

void Engine::NavMeshComponent::UpdateNavMesh()
{
	mNavMeshNeedsUpdate = true;
}

std::vector<glm::vec2> Engine::NavMeshComponent::CleanupPathfinding(const std::vector<Polygon>& triangles,
                                                            glm::vec2 start, glm::vec2 goal) const
{
	std::vector<glm::vec2> path{};

	// Check if there are enough triangles to form a path
	if (triangles.size() < 3)
	{
		return {};
	}

	// Initialize the path with the start position
	path.push_back(start);

	// Compute overlapping vertices between the first two triangles
	std::vector<std::vector<glm::vec2>> overlappingSides;
	for (auto currentTriangleVertex : triangles[0])
	{
		std::vector<glm::vec2> overlappingVertex;
		for (auto nextTriangleVertex : triangles[1])
		{
			if (currentTriangleVertex == nextTriangleVertex)
			{
				overlappingVertex.emplace_back(currentTriangleVertex);
			}

			if (overlappingVertex.size() == 2)
			{
				overlappingSides.emplace_back(overlappingVertex);
			}
		}
	}


	// Iterate through the remaining triangles
	for (uint32 i = 1; i < triangles.size(); i++)
	{
		std::vector<glm::vec2> overlappingVertex;
		for (const auto& currentTriangleVertex : triangles[i - 1])
		{
			for (const auto& nextTriangleVertex : triangles[i])
			{
				if (currentTriangleVertex != nextTriangleVertex)
				{
					continue;
				}

				if (currentTriangleVertex == nextTriangleVertex)
				{
					overlappingVertex.emplace_back(currentTriangleVertex);
				}

				if (overlappingVertex.size() == 2)
				{
					overlappingSides.emplace_back(overlappingVertex);
				}
			}
		}
	}

	for (const auto& currentSide : overlappingSides)
	{
		const glm::vec2 middlePoint = (currentSide[1] + currentSide[0]) / 2.0f;
		path.emplace_back(middlePoint);
	}

	path.emplace_back(goal);
	return path;
}

Engine::MetaType Engine::NavMeshComponent::Reflect()
{
	auto type = MetaType{MetaType::T<NavMeshComponent>{}, "NavMeshComponent"};
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	type.AddFunc(&NavMeshComponent::UpdateNavMesh, "UpdateNavMesh").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sCallFromEditorTag);
	ReflectComponentType<NavMeshComponent>(type);
	return type;
}

std::vector<glm::vec2> Engine::NavMeshComponent::FindQuickestPath(glm::vec2 startPos, glm::vec2 endPos) const
{
	// Initialize pointers to the start and end nodes
	const Node* startNode = nullptr;
	const Node* endNode = nullptr;

	// Find the start and end nodes based on their positions
	for (int i = 0; i < static_cast<int>(mPolygonDataNavMesh.size()); i++)
	{
		if (IsPointInsidePolygon({startPos[0], startPos[1]}, mPolygonDataNavMesh[i]))
		{
			startNode = &mAStarGraph.ListOfNodes[i];
		}
		if (IsPointInsidePolygon({endPos[0], endPos[1]}, mPolygonDataNavMesh[i]))
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
	std::vector<Polygon> trianglePathFound; // This stores the polygons corresponding to the node path

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
	pathFound = CleanupPathfinding(trianglePathFound, startPos, endPos);

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

	pathFound.push_back({endPos});
	return pathFound;
}

void Engine::NavMeshComponent::DebugDrawNavMesh(const World& world) const
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

			for (int j = 0; j < static_cast<int>(cleanedPolygonList[h].size()); j++)
			{
				if (j + 1 == static_cast<int>(cleanedPolygonList[h].size()))
				{
					// Draw a line connecting the last vertex to the first vertex
					DrawDebugLine(world, DebugCategory::AINavigation,
						{cleanedPolygonList[h][j].x, 0, cleanedPolygonList[h][j].y},
						{cleanedPolygonList[h][0].x, 0, cleanedPolygonList[h][0].y},
						colour);
				}
				else
				{
					// Draw a line connecting two consecutive vertices
					DrawDebugLine(world, DebugCategory::AINavigation,
						{cleanedPolygonList[h][j].x, 0, cleanedPolygonList[h][j].y},
						{
							cleanedPolygonList[h][j + 1].x, 0,
							cleanedPolygonList[h][j + 1].y
						},
						colour);
				}
			}
		}

		auto polygonDataNavMesh = navMesh.GetPolygonDataNavMesh();
		//explained and inspired by Ayoub's version though it has been heavily changed but the idea still remains.
		for (const auto& polygonList : polygonDataNavMesh)
		{
			// Draw the edges of each triangle with a blue color
			DrawDebugLine(world, DebugCategory::AINavigation, {polygonList[0].x, 0, polygonList[0].y},
				{polygonList[1].x, 0, polygonList[1].y},
				{0.f, 0.f, 1.f, 1.f});

			DrawDebugLine(world, DebugCategory::AINavigation, {polygonList[1].x, 0, polygonList[1].y},
				{polygonList[2].x, 0, polygonList[2].y},
				{0.f, 0.f, 1.f, 1.f});

			DrawDebugLine(world, DebugCategory::AINavigation, {polygonList[2].x, 0, polygonList[2].y},
				{polygonList[0].x, 0, polygonList[0].y},
				{0.f, 0.f, 1.f, 1.f});
		}
	}

	mAStarGraph.DebugDrawAStarGraph(world);
}
