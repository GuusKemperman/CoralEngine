#pragma once
#include <vector>
#include <glm/vec2.hpp>

#include "Edge.h"

namespace Engine
{
	class Node
	{
	public:
		/**
		 * \brief Constructor to initialize the node on the graph
		 * \param id The node's id
		 * \param position The node's position on the graph
		 */
		Node(int id, const glm::vec2& position);

		/// \brief 
		/// Adds an edge to the node, which connects to another node.
		/// \param toNode
		/// The node the edge connects to
		/// \param cost
		/// By default, if it's cost is less than 0, it'll calculate the heuristic, if the cost will be that of the given value.
		/// \param biDirectional
		/// By default, it'll create an edge in one direction unless otherwise specified, which in that case,
		/// it'll also create a similar edge but from the other node to the current one, AKA another edge in reverse.
		void AddEdge(const Node* toNode, float cost = -1, bool biDirectional = false);

		/// \brief Getter to grab the connecting edges to the node
		/// \return The connectingEdges as a vector of Edges
		[[nodiscard]] std::vector<Edge> GetConnectingEdges() const;

		/// \brief Getter to grab the position of the node
		/// \return The position as a vec2
		[[nodiscard]] glm::vec2 GetPosition() const;

		/// \brief Getter to grab the ID of the node
		/// \return The ID as an int
		[[nodiscard]] int GetId() const;

	private:
		/// \brief The node's Id
		int Id{};

		/// \brief The connecting edges to the node
		std::vector<Edge> ConnectingEdges{};

		/// \brief The position of the node
		glm::vec2 Position{};
	};
}
