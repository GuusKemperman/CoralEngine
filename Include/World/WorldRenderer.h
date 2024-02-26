#pragma once

namespace Engine
{
	class World;
	class FrameBuffer;
	class CameraComponent;

	struct DebugCategory
	{
		enum Enum
		{
			General = 1 << 0,
			Gameplay = 1 << 1,
			Physics = 1 << 2,
			Sound = 1 << 3,
			Rendering = 1 << 4,
			AINavigation = 1 << 5,
			AIDecision = 1 << 6,
			Editor = 1 << 7,
			AccelStructs = 1 << 8,
			Particles = 1 << 9,
			All = 0xFFFFFFFF
		};
	};

	struct Plane // for 2D debug drawing
	{
		enum Enum
		{
			XY = 0,
			XZ,
			YZ
		};
	};

	class WorldRenderer
	{
	public:
		WorldRenderer(const World& world);
		~WorldRenderer();

		void NewFrame();
		void Render();
		void Render(FrameBuffer& buffer, std::optional<glm::vec2> firstResizeBufferTo = {}, bool clearBufferFirst = true);

		const World& GetWorld() const { return mWorld; }

		glm::vec2 GetViewportSize() const { return mLastRenderedAtSize; }

		std::optional<std::pair<entt::entity, CameraComponent&>> GetMainCamera();
		std::optional<std::pair<entt::entity, const CameraComponent&>> GetMainCamera() const;

		void AddLine(DebugCategory::Enum category, glm::vec3 from, glm::vec3 to, glm::vec4 color) const;

		void AddLine(DebugCategory::Enum category, glm::vec2 from, glm::vec2 to, glm::vec4 color, Plane::Enum plane = Plane::XZ) const;

		void AddCircle(DebugCategory::Enum category, glm::vec3 center, float radius, glm::vec4 color, Plane::Enum plane = Plane::XZ) const;

		void AddSphere(DebugCategory::Enum category, glm::vec3 center, float radius, glm::vec4 color) const;

		void AddSquare(DebugCategory::Enum category, glm::vec3 center, float size, glm::vec4 color, Plane::Enum plane = Plane::XZ) const;

		void AddBox(DebugCategory::Enum category, glm::vec3 center, glm::vec3 halfExtends, glm::vec4 color)  const;

		void AddPolygon(DebugCategory::Enum category, std::vector<glm::vec3>& points, glm::vec4 color) const;

		static void SetDebugCategoryFlags(DebugCategory::Enum flags) { sDebugCategoryFlags = flags; }
		static DebugCategory::Enum GetDebugCategoryFlags() { return sDebugCategoryFlags; }

		glm::vec3 GetScreenToWorldDirection(glm::vec2 screenPosition) const;
		glm::vec3 ScreenToWorld(glm::vec2 screenPosition, float distanceFromCamera) const;

	private:
		void RenderAtSize(glm::vec2 size);

		// mWorld needs to be updated in World::World(World&&), so we give access to World to do so.
		friend class World;
		std::reference_wrapper<const World> mWorld;

		static inline DebugCategory::Enum sDebugCategoryFlags{};

		// In pixels
		glm::vec2 mLastRenderedAtSize{};
		glm::vec2 mLastRenderedAtPos{};

		mutable entt::entity mMainCamera{};
	};
}