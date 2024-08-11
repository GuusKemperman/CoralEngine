#pragma once

namespace CE
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

		glm::vec3 GetScreenToWorldDirection(glm::vec2 screenPosition) const;
		glm::vec3 ScreenToWorld(glm::vec2 screenPosition, float distanceFromCamera) const;
		glm::vec3 ScreenToWorldPlane(glm::vec2 screenPosition, float planeHeight) const;

	private:
		std::reference_wrapper<const World> mWorld;

		// In pixels
		glm::vec2 mLastRenderedAtSize{};
		glm::vec2 mLastRenderedAtPos{};
	};
}
