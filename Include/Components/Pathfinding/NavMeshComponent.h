#pragma once
#include "Geometry2d.hpp"
#include "Graph.h"
#include "Meta/MetaReflect.h"

namespace Engine
{
	class World;

	class NavMeshComponent
	{
	public:
		/**
		 * \brief Sets up the NavMesh based on the info from the file given.
		 */
		NavMeshComponent();

		void SetNavMesh(const World& world);

		/**
		 * \brief
		 * Finds the quickest path from one point to another, by doing an AStarSearch on the nav mesh.
		 * (which is then refined with the funnel algorithm)
		 * \param startPos starting position
		 * \param endPos ending position
		 * \return Returns all the coordinates of the quickest path found as a vector of glm::vec2
		 */
		[[nodiscard]] std::vector<glm::vec2> FindQuickestPath(const glm::vec2& startPos, const glm::vec2& endPos) const;

		[[nodiscard]] geometry2d::PolygonList GetCleanedPolygonList() const;

		[[nodiscard]] geometry2d::PolygonList GetPolygonDataNavMesh() const;

		/// \brief DebugDrawNavMesh, draws the NavMesh in order to be able to debug it
		/// \param world the current World in the ***REMOVED***ne
		void DebugDrawNavMesh(const World& world) const;

		bool mNavMeshNeedsUpdate = true;

		float mSizeX = 10;
		float mSizeY = 10;

		std::vector<glm::vec3> mSize{};

	private:
		/// \brief The A* Graph object.
		Graph mAStarGraph{};

		/// \brief The PolygonList that contains the walkable area. Only used for debugging.
		geometry2d::PolygonList mCleanedPolygonList;

		/// \brief The PolygonList that contains the triangles for the NavMesh after some CDT clean up.
		geometry2d::PolygonList mPolygonDataNavMesh;

		/**
		 * \brief Load the info from the given file in order to create the navmesh properly
		 * \return The walkable and obstacle areas
		 */
		[[nodiscard]] std::vector<geometry2d::PolygonList> LoadNavMeshData(const World& world) const;

		/**
		 * \brief
		 * Cleans up all the polygons through the use of the Clipper2 library, by getting the difference
		 * of the unions of the walkable polygons and the obstacle polygons. In order to get it ready for
		 * it to be triangulated.
		 * \param dirtyPolygonList The walkable and obstacle areas
		 */
		void CleanupGeometry(const std::vector<geometry2d::PolygonList>& dirtyPolygonList);

		/**
		 * \brief Triangulates the polygonList provided and sets up the dual graph between the nav mesh and the A* graph
		 * \param polygonList The walkable area / CleanedPolygonList
		 */
		void Triangulation(const geometry2d::PolygonList& polygonList);

		/**
		 * \brief Computes the shortest path within a list of triangles from the A* search function using funnel algorithm.
		 * \param triangles The A* search's quickest path
		 * \param start Starting position
		 * \param goal Ending position
		 * \return The new optimised quickest path found as a vector of glm::vec2
		 */
		[[nodiscard]] std::vector<glm::vec2> FunnelAlgorithm(const std::vector<geometry2d::Polygon>& triangles,
		                                                     const glm::vec2& start, const glm::vec2& goal) const;
		void UpdateNavMesh();

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(NavMeshComponent);
	};
}
