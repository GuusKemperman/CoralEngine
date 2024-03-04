#include "Precomp.h"
#include "Components/Pathfinding/Edge.h"

using namespace Engine;

Edge::Edge(const float cost, Node* toNode) : mCost(cost), mToNode(toNode)
{
}

float Edge::GetCost() const
{
	return mCost;
}

Node* Edge::GetToNode() const
{
	return mToNode;
}
