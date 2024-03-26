#pragma once

namespace Engine
{
	class World;
	class FrameBuffer;
	class CameraComponent;

	class WorldViewport
	{
	public:
		WorldViewport(const World& world);
		~WorldViewport();

		void UpdateSize(glm::vec2 size);

		const World& GetWorld() const { return mWorld; }

		glm::vec2 GetViewportSize() const { return mLastRenderedAtSize; }

		std::optional<std::pair<entt::entity, CameraComponent&>> GetMainCamera();
		std::optional<std::pair<entt::entity, const CameraComponent&>> GetMainCamera() const;

		void SetMainCamera(entt::entity entity) { mMainCamera = entity; }

		glm::vec3 GetScreenToWorldDirection(glm::vec2 screenPosition) const;
		glm::vec3 ScreenToWorld(glm::vec2 screenPosition, float distanceFromCamera) const;

	private:
		// mWorld needs to be updated in World::World(World&&), so we give access to World to do so.
		friend class World;
		std::reference_wrapper<const World> mWorld;

		// In pixels
		glm::vec2 mLastRenderedAtSize{};
		glm::vec2 mLastRenderedAtPos{};

		mutable entt::entity mMainCamera{};
	};
}
