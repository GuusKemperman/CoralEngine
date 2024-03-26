#include "Precomp.h"
#include "Components/Pathfinding/Node.h"

Engine::Node::Node(const int id, glm::vec2 position) : mId(id), mPosition(position)
{
}

void Engine::Node::AddEdge(Node* toNode, float cost, const bool biDirectional)
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

const std::vector<Engine::Edge>& Engine::Node::GetConnectingEdges() const
{
	return mConnectingEdges;
}

glm::vec2 Engine::Node::GetPosition() const
{
	return mPosition;
}

int Engine::Node::GetId() const
{
	return mId;
}
