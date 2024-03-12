#pragma once

namespace Engine
{
	class World;
	class DebugRenderer;
	class FrameBuffer;
	class CameraComponent;

	class WorldRenderer
	{
	public:
		WorldRenderer(const World& world);
		~WorldRenderer();

		void NewFrame();
		void Render();

#ifdef EDITOR
		void Render(FrameBuffer& buffer, std::optional<glm::vec2> firstResizeBufferTo = {},
		            bool clearBufferFirst = true);
#endif // EDITOR

		const World& GetWorld() const { return mWorld; }

		glm::vec2 GetViewportSize() const { return mLastRenderedAtSize; }

		std::optional<std::pair<entt::entity, CameraComponent&>> GetMainCamera();
		std::optional<std::pair<entt::entity, const CameraComponent&>> GetMainCamera() const;

		void SetMainCamera(entt::entity entity) { mMainCamera = entity; }

		glm::vec3 GetScreenToWorldDirection(glm::vec2 screenPosition) const;
		glm::vec3 ScreenToWorld(glm::vec2 screenPosition, float distanceFromCamera) const;

	private:
		void RenderAtSize(glm::vec2 size);

		// mWorld needs to be updated in World::World(World&&), so we give access to World to do so.
		friend class World;
		std::reference_wrapper<const World> mWorld;
		std::unique_ptr<DebugRenderer> mDebugRenderer{};

		// In pixels
		glm::vec2 mLastRenderedAtSize{};
		glm::vec2 mLastRenderedAtPos{};

		mutable entt::entity mMainCamera{};
	};
}
