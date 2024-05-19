#include "Precomp.h"
#include "Utilities/PathfindingInfo.h"

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

Pathfinding::Graph::Graph(const std::vector<Node>& nodes) : mNodes(nodes)
{
}

void Pathfinding::Graph::AddNode(const float x, const float y)
{
	const Node nodeToAdd = {static_cast<int>(mNodes.size()), {x, y}};
	mNodes.push_back(nodeToAdd);
}

std::vector<const Pathfinding::Node*> Pathfinding::Graph::AStarSearch(
	const Node* startNode, const Node* endNode) const
{
	struct OpenListItem
	{
		int mId;
		int mCameFromId{ -1 };
		float mG{};
		float mH{};
	};

	struct CompareNodes
	{
		bool operator()(const OpenListItem* lhs, const OpenListItem* rhs) const
		{
			return lhs->mG + lhs->mH > rhs->mG + rhs->mH;
		}
	};

	std::vector<OpenListItem> openStorage{};
	openStorage.resize(mNodes.size());

	for (int i = 0; i < static_cast<int>(openStorage.size()); i++)
	{
		openStorage[i].mId = i;
	}

	// Initialize an empty vector to store the resulting path
	std::vector<const Node*> nodePath = {};

	std::priority_queue<OpenListItem*, std::vector<OpenListItem*>, CompareNodes> open = {};
	
	// Create the OpenListItem for the start node and add it to the map and priority queue
	OpenListItem& startOpenListItem = openStorage[startNode->GetId()];
	startOpenListItem.mH = Heuristic(*startNode, *endNode);
	open.emplace(&startOpenListItem);

	while (!open.empty())
	{
		// Get the node with the lowest F (H + G) from the priority queue
		const OpenListItem& current = *open.top();
		open.pop();

		// If the current node is the end node, construct the path and return it
		if (current.mId == endNode->GetId())
		{
			const OpenListItem* pastItem = &current;

			while(true)
			{
				nodePath.emplace_back(&mNodes[pastItem->mId]);

				if (pastItem->mCameFromId == -1)
				{
					break;
				}
				pastItem = &openStorage[pastItem->mCameFromId];
			}

			std::reverse(nodePath.begin(), nodePath.end());
			return nodePath;
		}

		// Explore neighbouring nodes
		for (const Edge& edge : mNodes[current.mId].GetConnectingEdges())
		{
			const int toNodeId = edge.GetToNode()->GetId();
			OpenListItem& neighbour = openStorage[toNodeId];

			if (neighbour.mCameFromId != -1
				|| &neighbour == &startOpenListItem)
			{
				// Skip visited nodes
				continue;
			}

			// Calculate the new G (cost from start node to the current node)
			const auto newG = current.mG + edge.GetCost();

			// Update node information if this is a better path
			if (newG < neighbour.mG || neighbour.mG == 0.0f)
			{
				neighbour.mG = newG;
				neighbour.mH = Heuristic(mNodes[neighbour.mId], *endNode);
				neighbour.mCameFromId = current.mId;
				open.emplace(&neighbour);
			}
		}
	}
	return nodePath;
}

float Pathfinding::Graph::Heuristic(const Node& currentNode, const Node& endNode) const
{
	return glm::distance(endNode.GetPosition(), currentNode.GetPosition());
}

void Pathfinding::Graph::DebugDrawAStarGraph(const World& world) const
{
	for (const auto& n : mNodes)
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

