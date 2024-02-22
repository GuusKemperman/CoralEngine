#pragma once
#include <glm/vec4.hpp>

namespace Engine
{
	class Node;

	class Edge
	{
	public:
		/**
		 * \brief Constructor to initialize the node on the graph
		 * \param cost The edge's cost
		 * \param toNode the pointer of the node to which this edge connects to
		 */
		Edge(float cost, Node* toNode);

		/**
		 * \brief Getter to grab the cost of the edge
		 * \return The cost of the edge as a float
		 */
		[[nodiscard]] float GetCost() const;

		/**
		 * \brief Getter to grab the pointer of the node to which this edge connects to
		 * \return The node as a pointer
		 */
		[[nodiscard]] Node* GetToNode() const;

	private:
		/// \brief The edge's cost
		float Cost = 0;
		/// \brief The pointer of the node to which this edge connects to
		Node* ToNode = nullptr;
	};
}
