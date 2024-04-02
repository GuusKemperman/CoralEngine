#include "Precomp.h"
#include "Components/Pathfinding/PathfindingInfo.h"

#include <queue>
#include <unordered_map>

#include "World/World.h"
#include "Utilities/DrawDebugHelpers.h"

using namespace CE;

Pathfinding::Edge::Edge(const float cost, Node* toNode) : mCost(cost), mToNode(toNode)
{
}

float Pathfinding::Edge::GetCost() const
{
	return mCost;
}

Pathfinding::Node* Pathfinding::Edge::GetToNode() const
{
	return mToNode;
}

Pathfinding::Node::Node(const int id, glm::vec2 position) : mId(id), mPosition(position)
{
}

void Pathfinding::Node::AddEdge(Node* toNode, float cost, const bool biDirectional)
{
	if (cost < 0)
	{
		cost = sqrt(powf(toNode->mPosition.x - mPosition.x, 2) +
			powf(toNode->mPosition.y - mPosition.y, 2));
	}

	const Edge newEdge = {cost, toNode};
	mConnectingEdges.push_back(newEdge);

	if (biDirectional)
	{
		const auto otherNode = Edge(newEdge.GetCost(), this);
		newEdge.GetToNode()->mConnectingEdges.push_back(otherNode);
	}
}

const std::vector<Pathfinding::Edge>& Pathfinding::Node::GetConnectingEdges() const
{
	return mConnectingEdges;
}

glm::vec2 Pathfinding::Node::GetPosition() const
{
	return mPosition;
}

int Pathfinding::Node::GetId() const
{
	return mId;
}

Pathfinding::Graph::Graph(const std::vector<Node>& nodes) : ListOfNodes(nodes)
{
}

void Pathfinding::Graph::AddNode(const float x, const float y)
{
	const Node nodeToAdd = {static_cast<int>(ListOfNodes.size()), {x, y}};
	ListOfNodes.push_back(nodeToAdd);
}

std::vector<const Pathfinding::Node*> Pathfinding::Graph::AStarSearch(
	const Node* startNode, const Node* endNode) const
{
	// Initialize an empty vector to store the resulting path
	std::vector<const Node*> nodePath = {};

	// Initialize a map to keep track of OpenListItems and a priority queue to manage nodes
	std::unordered_map<int, OpenListItem> openListItemMap = {};
	std::priority_queue<OpenListItem*, std::vector<OpenListItem*>, CompareNodes> open = {};

	// Create the OpenListItem for the start node and add it to the map and priority queue
	OpenListItem startOpenListItem((startNode->GetId()), startNode);
	startOpenListItem.mH = Heuristic(*startNode, *endNode);
	openListItemMap[startNode->GetId()] = startOpenListItem;
	open.push(&openListItemMap[startNode->GetId()]);

	while (!open.empty())
	{
		// Get the node with the lowest F (H + G) from the priority queue
		OpenListItem* v = open.top();
		open.pop();

		// If the current node is the end node, construct the path and return it
		if (v->mActualNode == endNode)
		{
			const Node* pastNode = v->mActualNode;
			while (openListItemMap[pastNode->GetId()].mParentNode != nullptr)
			{
				nodePath.push_back(pastNode);
				pastNode = openListItemMap[pastNode->GetId()].mParentNode;
			}
			nodePath.push_back(startNode);
			std::reverse(nodePath.begin(), nodePath.end());
			return nodePath;
		}

		if (v->mVisited)
		{
			// Skip nodes that have already been visited
			continue;
		}
		v->mVisited = true;

		// Explore neighbouring nodes
		for (const auto edge : v->mActualNode->GetConnectingEdges())
		{
			const int toNodeId = edge.GetToNode()->GetId();
			const auto it = openListItemMap.find(toNodeId);
			OpenListItem* toNode;

			if (it == openListItemMap.end())
			{
				// Create a new OpenListItem and insert it into the map
				const OpenListItem newItem(toNodeId, edge.GetToNode());
				openListItemMap[toNodeId] = newItem;
				toNode = &openListItemMap[toNodeId];
			}
			else
			{
				// Node already exists in the map
				toNode = &it->second;
			}

			if (toNode->mVisited)
			{
				// Skip visited nodes
				continue;
			}

			// Calculate the new G (cost from start node to the current node)
			const auto newG = v->mG + edge.GetCost();

			// Update node information if this is a better path
			if (newG < toNode->mG || toNode->mG == 0.0f)
			{
				toNode->mG = newG;
				toNode->mH = Heuristic(*toNode->mActualNode, *endNode);
				toNode->mParentNode = v->mActualNode;
				open.push(toNode);
			}
		}
	}
	return nodePath;
}

float Pathfinding::Graph::Heuristic(const Node& currentNode, const Node& endNode) const
{
	return sqrt(
		static_cast<float>(pow(endNode.GetPosition().x - currentNode.GetPosition().x, 2)) + static_cast<float>(pow(
			endNode.GetPosition().y - currentNode.GetPosition().y, 2)));
}

void Pathfinding::Graph::DebugDrawAStarGraph(const World& world) const
{
	for (const auto& n : ListOfNodes)
	{
		DrawDebugCircle(world, DebugCategory::Gameplay, glm::vec3{n.GetPosition().x, 0, n.GetPosition().y},
		                0.2f,
		                {1.f, 0.f, 0.f, 1.f});
		for (const auto& m : n.GetConnectingEdges())
		{
			DrawDebugLine(world, DebugCategory::Gameplay, glm::vec3{n.GetPosition().x, 0, n.GetPosition().y},
			              glm::vec3{
				              m.GetToNode()->GetPosition().x, 0, m.GetToNode()->GetPosition().y
			              }, {0.f, 1.f, 0.f, 1.f});
		}
	}
}

bool Pathfinding::Graph::CompareNodes::operator()(const OpenListItem* lhs, const OpenListItem* rhs) const
{
	return lhs->mG + lhs->mH > rhs->mG + rhs->mH;
}
