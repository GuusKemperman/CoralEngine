#pragma once
#include "Core/EngineSubsystem.h"

namespace CE
{
	class Engine;
	class World;
	class FrameBuffer;
	class DebugRenderer;
	class ISubRenderer;

	class Renderer final :
		public EngineSubsystem<Renderer>
	{
		friend EngineSubsystem;
		friend Engine;

		Renderer();
		~Renderer() final override;

	public:
		void Render(const World& world, FrameBuffer& buffer);

#ifdef EDITOR
		void RenderToFrameBuffer(
			const World& world,
			FrameBuffer& buffer, 
			std::optional<glm::vec2> firstResizeBufferTo = {},
			bool clearBufferFirst = true);
#endif // EDITOR

		const DebugRenderer& GetDebugRenderer() const { return *mDebugRenderer; };

	private:
		void Render(const World& world, glm::vec2 viewportSize, FrameBuffer& buffer);

		std::unique_ptr<ISubRenderer> mMeshRenderer;
		std::unique_ptr<ISubRenderer> mShadowMapRenderer;
		std::unique_ptr<ISubRenderer> mUIRenderer;
		std::unique_ptr<DebugRenderer> mDebugRenderer;
	};
}

