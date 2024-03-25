#pragma once
#include "Core/EngineSubsystem.h"

namespace Engine
{
	class World;
	class FrameBuffer;
	class MeshRenderer;
	class UIRenderer;
	class DebugRenderer;

	class Renderer final :
		public EngineSubsystem<Renderer>
	{
		friend EngineSubsystem;
		friend EngineClass;

		Renderer();
		~Renderer() final override;

	public:
		void Render(const World& world);

		void RenderToFrameBuffer(
			const World& world,
			FrameBuffer& buffer, 
			std::optional<glm::vec2> firstResizeBufferTo = {},
			bool clearBufferFirst = true);

		const DebugRenderer& GetDebugRenderer() const { return *mDebugRenderer; };

	private:
		void Render(const World& world, glm::vec2 viewportSize);

		std::unique_ptr<MeshRenderer> mMeshRenderer;
		std::unique_ptr<UIRenderer> mUIRenderer;
		std::unique_ptr<DebugRenderer> mDebugRenderer;
	};
}

