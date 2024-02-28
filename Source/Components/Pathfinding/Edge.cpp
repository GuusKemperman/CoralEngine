#include "Precomp.h"
#include "Components/Pathfinding/Edge.h"

using namespace Engine;

Edge::Edge(const float cost, Node* toNode) : Cost(cost), ToNode(toNode)
{
}

float Edge::GetCost() const
{
	return Cost;
}

Node* Edge::GetToNode() const
{
	return ToNode;
}
