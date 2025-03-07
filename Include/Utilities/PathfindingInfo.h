#pragma once

namespace CE
{
	class World;

	class Pathfinding
	{
	public:
		class Edge;

		class Node
		{
		public:
			/**
			 * \brief Constructor to initialize the node on the graph
			 * \param id The node's id
			 * \param position The node's position on the graph
			 */
			Node(int id, glm::vec2 position);

			/// \brief 
			/// Adds an edge to the node, which connects to another node.
			/// \param toNode
			/// The node the edge connects to
			/// \param cost
			/// By default, if it's cost is less than 0, it'll calculate the heuristic, if the cost will be that of the given value.
			/// \param biDirectional
			/// By default, it'll create an edge in one direction unless otherwise specified, which in that case,
			/// it'll also create a similar edge but from the other node to the current one, AKA another edge in reverse.
			void AddEdge(Node* toNode, float cost = -1, bool biDirectional = false);

			/// \brief Getter to grab the connecting edges to the node
			/// \return The connectingEdges as a vector of Edges
			[[nodiscard]] const std::vector<Edge>& GetConnectingEdges() const;


			/// \brief Getter to grab the position of the node
			/// \return The position as a vec2
			[[nodiscard]] glm::vec2 GetPosition() const;

			/// \brief Getter to grab the ID of the node
			/// \return The ID as an int
			[[nodiscard]] int GetId() const;

		private:
			/// \brief The node's Id
			int mId{};

			/// \brief The connecting edges to the node
			std::vector<Edge> mConnectingEdges{};

			/// \brief The position of the node
			glm::vec2 mPosition{};
		};

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
			float mCost = 0;
			/// \brief The pointer of the node to which this edge connects to
			Node* mToNode = nullptr;
		};

		class Graph
		{
		public:
			Graph() = default;

			/**
			 * \brief Constructor to initialize the A* graph
			 * \param nodes The nodes that belong to this graph
			 */
			explicit Graph(const std::vector<Node>& nodes);

			/// \brief The vector containing all the nodes belonging to this graph
			std::vector<Node> mNodes{};

			/**
			 * \brief AddNode, creates a default node based on it's location.
			 * \param x X position
			 * \param y Y position
			 */
			void AddNode(float x, float y);

			/**
			 * \brief
			 * AStarSearch, returns the quickest path from startNode to endNode through the use of the A* search algorithm.
			 * \param startNode Starting node
			 * \param endNode Ending node
			 * \return The vector returned contains the nodes that you must go through to reach the end in the quickest way possible.
			 */
			std::vector<const Node*> AStarSearch(const Node* startNode, const Node* endNode) const;


			/// \brief DebugDrawAStarGraph, draws the A* Graph in order to be able to debug it
			void DebugDrawAStarGraph(const World& world) const;

		private:
			/**
			 * \brief Heuristic calculates the heuristic of the node, aka, the distance between the current Node and the endNode.
			 * \param currentNode The current node you're on
			 * \param endNode The ending node
			 * \return The heuristic estimate as a float
			 */
			[[nodiscard]] float Heuristic(const Node& currentNode, const Node& endNode) const;
		};
	};
}
