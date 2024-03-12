#pragma once

#include "Node.h"

namespace Engine
{
	class World;

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
		std::vector<Node> ListOfNodes{};

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

		/**
		 * \brief
		 * The OpenListItem struct is used in order to store info for each node.
		 * Objects of this type are created each time the AStarSearch function is called upon in order to,
		 * make sure that info between each iteration of AStarSearch per agent won't mess with each other
		 */
		struct OpenListItem
		{
			float mG = 0;
			float mH = 0;
			int mId{};

			bool mVisited = false;

			const Node* mActualNode = nullptr;
			const Node* mParentNode = nullptr;

			OpenListItem()
			{
				// Initialize members if needed
				mId = 0;
			}

			/**
			 * \brief Constructor to initialize the open list item
			 * \param id The node's id
			 * \param actualNode The open list item's node pointer
			 */
			OpenListItem(const int id, const Node* actualNode) : mId(id), mActualNode(actualNode)
			{
			}
		};

		/**
		 * \brief
		 * CompareNodes is a struct containing a boolean operator used to define how to compare OpenListItems
		 * within the priority queue used in the AStarSearch.
		 */
		struct CompareNodes
		{
			bool operator()(const OpenListItem* lhs, const OpenListItem* rhs) const;
		};
	};
}
