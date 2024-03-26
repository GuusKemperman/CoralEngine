#pragma once
#include "Geometry2d.h"
#include "Graph.h"
#include "Meta/MetaReflect.h"

namespace Engine
{
	class TransformComponent;
	class World;

	class NavMeshComponent
	{
	public:
		void GenerateNavMesh(const World& world);

		/**
		 * \brief
		 * Finds the quickest path from one point to another, by doing an AStarSearch on the nav mesh.
		 * (which is then refined with the funnel algorithm)
		 * \param startPos starting position
		 * \param endPos ending position
		 * \return Returns all the coordinates of the quickest path found as a vector of glm::vec2
		 */
		[[nodiscard]] std::vector<glm::vec2> FindQuickestPath(glm::vec2 startPos, glm::vec2 endPos) const;

		[[nodiscard]] const std::vector<TransformedPolygon>& GetCleanedPolygonList() const { return mCleanedPolygonList; }

		[[nodiscard]] const std::vector<TransformedPolygon>& GetPolygonDataNavMesh() const { return mPolygonDataNavMesh; }

		/// \brief DebugDrawNavMesh, draws the NavMesh in order to be able to debug it
		/// \param world the current World in the ***REMOVED***ne
		void DebugDrawNavMesh(const World& world) const;

		bool mNavMeshNeedsUpdate = true;

		float mSpaceBetweenTerrainSamples = 1.0f;

		size_t mMaxNumOfTerrainSamples = 32768;

	private:
		/// \brief The A* Graph object.
		Graph mAStarGraph{};

		/// \brief The std::vector<PolygonPoints> that contains the walkable area. Only used for debugging.
		std::vector<TransformedPolygon> mCleanedPolygonList;

		/// \brief The std::vector<PolygonPoints> that contains the triangles for the NavMesh after some CDT clean up.
		std::vector<TransformedPolygon> mPolygonDataNavMesh;

		/**
		 * \brief Load the info from the given file in order to create the navmesh properly
		 * \return The walkable and obstacle areas
		 */
		struct NavMeshData
		{
			std::vector<TransformedPolygon> mWalkable{};
			std::vector<TransformedPolygon> mObstacles{};
		};
		[[nodiscard]] NavMeshData GenerateNavMeshData(const World& world) const;

		/**
		 * \brief
		 * Cleans up all the polygons through the use of the Clipper2 library, by getting the difference
		 * of the unions of the walkable polygons and the obstacle polygons. In order to get it ready for
		 * it to be triangulated.
		 */
		static std::vector<TransformedPolygon> GetDifferences(const NavMeshData& navMeshData);

		/**
		 * \brief Triangulates the polygonList provided and sets up the dual graph between the nav mesh and the A* graph
		 * \param polygonList The walkable area / CleanedPolygonList
		 */
		void Triangulation(const std::vector<TransformedPolygon>& polygonList);

		/**
		 * \brief Computes the shortest path within a list of triangles from the A* search function using funnel algorithm.
		 * \param triangles The A* search's quickest path
		 * \param start Starting position
		 * \param goal Ending position
		 * \return The new optimised quickest path found as a vector of glm::vec2
		 */
		[[nodiscard]] std::vector<glm::vec2> FunnelAlgorithm(const std::vector<PolygonPoints>& triangles,
		                                                     glm::vec2 start, glm::vec2 goal) const;
		void UpdateNavMesh();

		std::vector<glm::vec2> CleanupPathfinding(const std::vector<TransformedPolygon>& triangles,
		                                          glm::vec2 start, glm::vec2 goal) const;

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(NavMeshComponent);
	};
}
