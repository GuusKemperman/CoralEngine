#pragma once
#include "Meta/MetaReflect.h"

namespace CE
{
	class BinaryGSONObject;
	class World;
	class TransformComponent;

	class CameraComponent
	{	
	public:	
		const glm::mat4& GetViewProjection() const { return mViewProjection; }
		const glm::mat4& GetView() const { return mView; }
		const glm::mat4& GetProjection() const { return mProjection; }
		const glm::mat4& GetOrthographicProjection() const { return mOrthographicProjection; }

		void UpdateView(glm::vec3 position, glm::vec3 forward, glm::vec3 up, bool recalulateViewProjection = true);
		void UpdateView(const TransformComponent& transform, bool recalulateViewProjection = true);
		void UpdateProjection(glm::vec2 viewportSize, bool recalculateViewProjection = true);
		void UpdateProjection(float aspectRatio, bool recalculateViewProjection = true);

		// By default, the view/proj are recalculated at the end of every frame.
		void RecalculateViewProjection();

		/**
		 * \brief Returns the selected camera. If none is selected, will lazily select one.
		 * \return entt::null if no camera in the world, otherwise the owner of the camera. The owner is guaranteed to have a camera attached to it.
		 */
		static entt::entity GetSelected(const World& world);
		static bool IsSelected(const World& world, entt::entity cameraOwner);

		static void Select(World& world, entt::entity cameraOwner);
		static void Deselect(World& world);

		float mFar = 5000.0f;		
		float mNear = 1.f;
		float mFOV = glm::radians(45.0f);
		glm::vec2 mViewportSize;

		glm::mat4 mView{};
		glm::mat4 mProjection{};
		glm::mat4 mOrthographicProjection{};
		glm::mat4 mViewProjection{};
		glm::mat4 mInvViewProjection{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(CameraComponent);
	};

	class CameraSelectedTag
	{
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(CameraSelectedTag);
	};
}