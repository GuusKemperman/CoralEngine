//#include "Precomp.h"
//#include "Utilities/FrameBuffer.h"
//
//Engine::FrameBuffer::FrameBuffer(const glm::ivec2 initialSize)
//{
//	glGenFramebuffers(1, &mFrameBuffer);
//	Bind();
//
//	glGenTextures(1, &mColorTexture);
//	glGenRenderbuffers(1, &mDepthTexture);
//
//	Resize(initialSize);
//
//	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mColorTexture, 0);
//
//	const GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
//	glDrawBuffers(1, DrawBuffers);
//
//	CheckGL();
//	CheckFrameBuffer();
//	Unbind();
//}
//
//Engine::FrameBuffer::~FrameBuffer()
//{
//	glDeleteFramebuffers(1, &mFrameBuffer);
//	glDeleteTextures(1, &mColorTexture);
//	glDeleteRenderbuffers(1, &mDepthTexture);
//	CheckGL();
//}
//
//void Engine::FrameBuffer::Bind()
//{
//	CheckGL();
//	glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer);
//	CheckGL();
//	SetGLViewport();
//}
//
//void Engine::FrameBuffer::Unbind()
//{
//	CheckGL();
//
//	glBindFramebuffer(GL_FRAMEBUFFER, 0);
//
//	// Reset the viewport
//	const glm::vec2 displaySize = ImGui::GetIO().DisplaySize;
//
//	if (displaySize.x > 1.0f
//		&& displaySize.y > 1.0f)
//	{
//		glViewport(0, 0, static_cast<GLsizei>(displaySize.x), static_cast<GLsizei>(displaySize.y));
//	}
//
//	CheckGL();
//}
//
//void Engine::FrameBuffer::Resize(const glm::ivec2 newSize)
//{
//	if (mSize == newSize)
//	{
//		return;
//	}
//
//	mSize = newSize;
//
//	glBindTexture(GL_TEXTURE_2D, mColorTexture);
//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, mSize.x, mSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
//	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mColorTexture, 0);
//
//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mSize.x, mSize.y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mColorTexture, 0);
//
//	glBindRenderbuffer(GL_RENDERBUFFER, mDepthTexture);
//	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, mSize.x, mSize.y);
//	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthTexture);
//
//	CheckGL();
//	CheckFrameBuffer();
//
//	SetGLViewport();
//}
//
//void Engine::FrameBuffer::Clear()
//{
//	glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a);
//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//	CheckGL();
//}
//
//void Engine::FrameBuffer::SetGLViewport() const
//{
//	glViewport(0, 0, mSize.x, mSize.y);
//	CheckGL();
//}
