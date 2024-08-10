#include "Precomp.h"
#include "Rendering/FrameBuffer.h"

#include "Core/Renderer.h"

CE::FrameBuffer::FrameBuffer(glm::vec2 initialSize) :
	mSize(initialSize),
	mImpl(Renderer::Get().CreateFrameBufferPlatformImpl())
{
}
