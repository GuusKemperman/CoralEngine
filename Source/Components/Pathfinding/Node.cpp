#include "Precomp.h"
#include "Components/Pathfinding/Node.h"

using namespace Engine;

Node::Node(const int id, const glm::vec2& position) : mId(id), mPosition(position)
{
}

void Node::AddEdge(const Node* toNode, float cost, const bool biDirectional)
{
	if (cost < 0)
	{
		cost = sqrt(static_cast<float>(pow(toNode->mPosition.x - mPosition.x, 2)) +
			static_cast<float>(pow(toNode->mPosition.y - mPosition.y, 2)));
	}

	const Edge newEdge = {cost, const_cast<Node*>(toNode)};
	mConnectingEdges.push_back(newEdge);

	if (biDirectional)
	{
		const auto otherNode = Edge(newEdge.GetCost(), this);
		newEdge.GetToNode()->mConnectingEdges.push_back(otherNode);
	}
}

std::vector<Edge> Node::GetConnectingEdges() const
{
	return mConnectingEdges;
}

glm::vec2 Node::GetPosition() const
{
	return mPosition;
}

int Node::GetId() const
{
	return mId;
}
