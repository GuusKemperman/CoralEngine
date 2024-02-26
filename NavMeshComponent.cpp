#include "Precomp.h"
#include "NavMeshComponent.h"

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
#include "Utilities/Reflect/ReflectComponentType.h"
#include "World/Registry.h"
#include "World/World.h"
#include "World/WorldRenderer.h"

using namespace Engine;

NavMeshComponent::NavMeshComponent()
{
	//CleanupGeometry(LoadNavMeshData(filePath));
	//Triangulation(CleanedPolygonList);
}

// Discovered the tokenisation of lines from ChatGPT, but i then changed all the code for it to work with FileIO
//std::vector<geometry2d::PolygonList> NavMeshComponent::LoadNavMeshData() const
//{
//// Initialize a vector to store the messy polygons
//std::vector<geometry2d::PolygonList> messyPolygons;

//// Read the content of the file into a string
//std::string readFileString = bee::Engine.FileIO().ReadTextFile(bee::FileIO::Directory::Asset, filePath);

//// Create an input string stream to split the input into lines
//std::istringstream iss(readFileString);

//// Check if the file is empty
//if (!bee::Engine.FileIO().Exists(bee::FileIO::Directory::Asset, filePath))
//{
//	spdlog::info("File not Found");
//	return {};
//}

//// Initialize lists to store walkable and obstacle polygons
//bee::geometry2d::PolygonList walkableList = {};
//bee::geometry2d::PolygonList obstacleList = {};

//// Iterate through each line in the file
//std::string line;
//while (std::getline(iss, line))
//{
//	// Create a string stream for the current line
//	std::istringstream lineStream(line);
//	std::vector<std::string> tokens;
//	std::string token;

//	// Tokenize the line and store each token
//	while (lineStream >> token)
//	{
//		tokens.push_back(token);
//	}

//	for (size_t i = 0; i < tokens.size(); ++i)
//	{
//		// Check if 'o' or 'w' is in the tokens, indicating an obstacle or walkable area
//		if (tokens[i] == "o" || tokens[i] == "w")
//		{
//			bee::geometry2d::Polygon polygon;
//			// Extract every two numbers following 'o' or 'w' and create a polygon
//			for (size_t j = i + 1; j < tokens.size(); j += 2)
//			{
//				if (j + 1 < tokens.size())
//				{
//					const float num1 = std::stof(tokens[j]);
//					const float num2 = std::stof(tokens[j + 1]);
//					const glm::vec2 pair = {num1, num2};
//					polygon.push_back(pair);
//				}
//			}

//			if (tokens[i] == "o")
//			{
//				// Create components for obstacle entity and add it to the obstacle list
//				const auto entity = bee::Engine.ECS().CreateEntity();
//				bee::Engine.ECS().CreateComponent<Physics::PolygonCollider>(entity, polygon);
//				bee::Engine.ECS().CreateComponent<Physics::Body>(entity, 0, polygon, Physics::Body::Static);

//				obstacleList.push_back(polygon);
//			}
//			else
//			{
//				// Add the polygon to the walkable list
//				walkableList.push_back(polygon);
//			}
//		}
//		else if (tokens[i] == "p" || tokens[i] == "a")
//		{
//			glm::vec2 playerPosition;
//			for (size_t j = i + 1; j < tokens.size(); j += 2)
//			{
//				if (j + 1 < tokens.size())
//				{
//					playerPosition = {std::stof(tokens[j]), std::stof(tokens[j + 1])};
//				}
//			}

//			// Create a NavMesh agent or player entity
//			CreateAgentOrPlayer(tokens[i] == "p", playerPosition);
//		}
//	}
//}

//// Add the walkable and obstacle lists to the messy polygons vector
//messyPolygons.push_back(walkableList);
//messyPolygons.push_back(obstacleList);

//return messyPolygons;
//}

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
	CleanedPolygonList.clear();
	CleanedPolygonList = tempPolygonList;
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
		PolygonDataNavMesh.push_back(polygon);

		// Calculate the center of the triangle and add it as a node to AStarGraph
		const glm::vec2 centerOfTriangle = geometry2d::ComputeCenterOfPolygon(polygon);
		AStarGraph.AddNode(centerOfTriangle.x, centerOfTriangle.y);
	}

	// Create edges between nodes based on triangle neighbors
	for (int k = 0; k < static_cast<int>(cdt.triangles.size()); k++)
	{
		for (int j = 0; j < 3; j++)
		{
			if (cdt.triangles[k].neighbors[j] < cdt.triangles.size())
			{
				AStarGraph.ListOfNodes[k].AddEdge(&AStarGraph.ListOfNodes[cdt.triangles[k].neighbors[j]]);
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
	for (int i = 2; i < triangles.size(); i++)
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
		for (int j = 0; j < overlappingVertexes.size(); j++)
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
		for (int i = 0; i < funnelLeft.size(); i++)
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
		for (int i = 0; i < funnelRight.size(); i++)
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
}

MetaType NavMeshComponent::Reflect()
{
	auto type = MetaType{MetaType::T<NavMeshComponent>{}, "NavMeshComponent"};
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	type.AddField(&NavMeshComponent::m_SizeX, "SizeX").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&NavMeshComponent::m_SizeY, "SizeZ").GetProperties().Add(Props::sIsScriptableTag);
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
	for (int i = 0; i < static_cast<int>(PolygonDataNavMesh.size()); i++)
	{
		if (geometry2d::IsPointInsidePolygon({startPos[0], startPos[1]}, PolygonDataNavMesh[i]))
		{
			startNode = &AStarGraph.ListOfNodes[i];
		}
		if (geometry2d::IsPointInsidePolygon({endPos[0], endPos[1]}, PolygonDataNavMesh[i]))
		{
			endNode = &AStarGraph.ListOfNodes[i];
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
		nodePathFound = AStarGraph.AStarSearch(startNode, endNode);

		// Convert nodes to corresponding polygons
		for (const auto& node : nodePathFound)
		{
			trianglePathFound.push_back(PolygonDataNavMesh[node->GetId()]);
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
	return CleanedPolygonList;
}

geometry2d::PolygonList NavMeshComponent::GetPolygonDataNavMesh() const
{
	return PolygonDataNavMesh;
}

void NavMeshComponent::DebugDrawNavMesh(const World& world) const
{
	const auto& view = world.GetRegistry().View<NavMeshComponent>();
	if (view.empty()) { return; }

	//const auto* transformView = world.GetRegistry().TryGet<TransformComponent>(view.front());

	for (auto& agentId : view)
	{
		const auto* transformView = world.GetRegistry().TryGet<TransformComponent>(agentId);
		auto [navMesh] = view.get(agentId);

		auto cleanedPolygonList = navMesh.GetCleanedPolygonList();

		if (transformView == nullptr)
		{
			world.GetRenderer().AddLine(DebugCategory::AINavigation, {m_SizeX / 2, -m_SizeY / 2},
			                            {m_SizeX / 2, m_SizeY / 2},
			                            {1.f, 0.f, 0.f, 1.f});
			world.GetRenderer().AddLine(DebugCategory::AINavigation, {m_SizeX / 2, m_SizeY / 2},
			                            {-m_SizeX / 2, m_SizeY / 2},
			                            {1.f, 0.f, 0.f, 1.f});
			world.GetRenderer().AddLine(DebugCategory::AINavigation, {-m_SizeX / 2, m_SizeY / 2},
			                            {-m_SizeX / 2, -m_SizeY / 2},
			                            {1.f, 0.f, 0.f, 1.f});
			world.GetRenderer().AddLine(DebugCategory::AINavigation, {-m_SizeX / 2, -m_SizeY / 2},
			                            {m_SizeX / 2, -m_SizeY / 2},
			                            {1.f, 0.f, 0.f, 1.f});
		}
		else
		{
			world.GetRenderer().AddLine(DebugCategory::AINavigation, {
				                            m_SizeX / 2 + transformView->GetWorldPosition().x,
				                            -m_SizeY / 2 + transformView->GetWorldPosition().y
			                            },
			                            {
				                            m_SizeX / 2 + transformView->GetWorldPosition().x,
				                            m_SizeY / 2 + transformView->GetWorldPosition().y
			                            },
			                            {1.f, 0.f, 0.f, 1.f});
			world.GetRenderer().AddLine(DebugCategory::AINavigation, {
				                            m_SizeX / 2 + transformView->GetWorldPosition().x,
				                            m_SizeY / 2 + transformView->GetWorldPosition().y
			                            },
			                            {
				                            -m_SizeX / 2 + transformView->GetWorldPosition().x,
				                            m_SizeY / 2 + transformView->GetWorldPosition().y
			                            },
			                            {1.f, 0.f, 0.f, 1.f});
			world.GetRenderer().AddLine(DebugCategory::AINavigation, {
				                            -m_SizeX / 2 + transformView->GetWorldPosition().x,
				                            m_SizeY / 2 + transformView->GetWorldPosition().y
			                            },
			                            {
				                            -m_SizeX / 2 + transformView->GetWorldPosition().x,
				                            -m_SizeY / 2 + transformView->GetWorldPosition().y
			                            },
			                            {1.f, 0.f, 0.f, 1.f});
			world.GetRenderer().AddLine(DebugCategory::AINavigation, {
				                            -m_SizeX / 2 + transformView->GetWorldPosition().x,
				                            -m_SizeY / 2 + transformView->GetWorldPosition().y
			                            },
			                            {
				                            m_SizeX / 2 + transformView->GetWorldPosition().x,
				                            -m_SizeY / 2 + transformView->GetWorldPosition().y
			                            },
			                            {1.f, 0.f, 0.f, 1.f});
		}

		for (int h = 0; h < static_cast<int>(cleanedPolygonList.size()); h++)
		{
			// Choose a color for the polygon, green for the first one and red for others
			const glm::vec4 colour = {1.f, (h == 0 ? 1.f : 0.f), 0.f, 1.f};

			for (int j = 0; j < static_cast<int>(cleanedPolygonList[h].size()); j++)
			{
				if (j + 1 == static_cast<int>(cleanedPolygonList[h].size()))
				{
					// Draw a line connecting the last vertex to the first vertex


					world.GetRenderer().AddLine(DebugCategory::Gameplay, cleanedPolygonList[h][j],
					                            cleanedPolygonList[h][0], colour);
				}
				else
				{
					// Draw a line connecting two consecutive vertices
					world.GetRenderer().AddLine(DebugCategory::Gameplay, cleanedPolygonList[h][j],
					                            cleanedPolygonList[h][j + 1], colour);
				}
			}
		}

		auto polygonDataNavMesh = navMesh.GetPolygonDataNavMesh();
		//explained and inspired by Ayoub's version though it has been heavily changed but the idea still remains.
		for (const auto& polygonList : polygonDataNavMesh)
		{
			// Draw the edges of each triangle with a blue color
			world.GetRenderer().AddLine(DebugCategory::Gameplay, polygonList[0], polygonList[1],
			                            {0.f, 0.f, 1.f, 1.f});
			world.GetRenderer().AddLine(DebugCategory::Gameplay, polygonList[1], polygonList[2],
			                            {0.f, 0.f, 1.f, 1.f});
			world.GetRenderer().AddLine(DebugCategory::Gameplay, polygonList[2], polygonList[0],
			                            {0.f, 0.f, 1.f, 1.f});
		}
	}

	AStarGraph.DebugDrawAStarGraph(world);
}
