#ifdef EDITOR
#pragma once
#ifdef PLATFORM_WINDOWS
#include "Platform/PC/Rendering/FramebufferPC.h"
#endif//PLATFORM_WINDOWS


//
//namespace Engine
//{
//	class MyShader;
//
//	class FrameBuffer
//	{
//	public:
//		FrameBuffer(glm::ivec2 initialSize = {1, 1});
//		~FrameBuffer();
//
//		void Bind();
//		void Unbind();
//
//		void Resize(glm::ivec2 newSize);
//		glm::ivec2 GetSize() const { return mSize; }
//
//		void Clear();
//
//		void SetClearColor(glm::vec4 color) { mClearColor = color; }
//		const glm::vec4 GetClearColor() { return mClearColor; }
//
//		GLuint GetColorTextureId() { return mColorTexture; }
//
//	private:
//		void SetGLViewport() const;
//
//		GLuint mFrameBuffer{};
//		GLuint mColorTexture{};
//		GLuint mDepthTexture{};
//
//		glm::vec4 mClearColor{};
//		glm::ivec2 mSize{};
//	};
//}
#endif // EDITOR