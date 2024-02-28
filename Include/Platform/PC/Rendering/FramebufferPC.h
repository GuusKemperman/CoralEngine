#ifdef EDITOR
#pragma once
#include "DX12Classes/DXDefines.h"
#include "glm/glm.hpp"
#include "DX12Classes/DXResource.h"

namespace Engine
{
	class MyShader;

	class FrameBuffer
	{
	public:
		FrameBuffer(glm::ivec2 initialSize = {1, 1});
		~FrameBuffer();

		void Bind();
		void Unbind();

		void Resize(glm::ivec2 newSize);
		glm::ivec2 GetSize() const { return mSize; }

		void Clear();

		void SetClearColor(glm::vec4 color) { mClearColor = color; }
		const glm::vec4 GetClearColor() { return mClearColor; }

		//GLuint GetColorTextureId() { return mColorTexture; }

	private:
		/*void SetGLViewport() const;*/

		//GLuint mFrameBuffer{};
		//GLuint mColorTexture{};
		//GLuint mDepthTexture{};
		std::unique_ptr<DXResource> resource[FRAME_BUFFER_COUNT];
		unsigned int frameBufferIndex[FRAME_BUFFER_COUNT];

		glm::vec4 mClearColor{};
		glm::ivec2 mSize{};
	};
}
#endif // EDITOR