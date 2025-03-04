#pragma once
#include "Meta/MetaReflect.h"

namespace CE
{
	class World;

	class CameraComponent
	{	
	public:	
		/**
		 * \brief Returns the selected camera. If none is selected, will lazily select one.
		 * \return entt::null if no camera in the world, otherwise the owner of the camera. The owner is guaranteed to have a camera attached to it.
		 */
		static entt::entity GetSelected(const World& world);
		static bool IsSelected(const World& world, entt::entity cameraOwner);

		static const CameraComponent* TryGetSelectedCamera(const World& world);
		static CameraComponent* TryGetSelectedCamera(World& world);

		static void Select(World& world, entt::entity cameraOwner);
		static void Deselect(World& world);

		float mFar = 5000.0f;		
		float mNear = 1.0f;

		float mFOV = glm::radians(45.0f);
		float mOrthoGraphicSize = 5.0f;

		glm::mat4 mView{};
		glm::mat4 mProjection{};
		glm::mat4 mViewProjection{};

		bool mIsOrthoGraphic{};

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