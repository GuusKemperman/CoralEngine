#include "Precomp.h"
#include "Node.h"

using namespace Engine;

Node::Node(const int id, const glm::vec2& position) : Id(id), Position(position)
{
}

void Node::AddEdge(const Node* toNode, float cost, const bool biDirectional)
{
	if (cost < 0)
	{
		cost = sqrt(static_cast<float>(pow(toNode->Position.x - Position.x, 2)) +
			static_cast<float>(pow(toNode->Position.y - Position.y, 2)));
	}

	const Edge newEdge = {cost, const_cast<Node*>(toNode)};
	ConnectingEdges.push_back(newEdge);

	if (biDirectional)
	{
		const auto otherNode = Edge(newEdge.GetCost(), this);
		newEdge.GetToNode()->ConnectingEdges.push_back(otherNode);
	}
}

std::vector<Edge> Node::GetConnectingEdges() const
{
	return ConnectingEdges;
}

glm::vec2 Node::GetPosition() const
{
	return Position;
}

int Node::GetId() const
{
	return Id;
}
