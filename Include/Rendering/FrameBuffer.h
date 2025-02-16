#pragma once

namespace CE
{
	struct FrameBufferPlatformImpl;

	class FrameBuffer
	{
	public:
		FrameBuffer(glm::vec2 initialSize = { 1.0f, 1.0f });

		FrameBuffer(FrameBuffer&&) noexcept = default;
		FrameBuffer(const FrameBuffer&) = delete;

		FrameBuffer& operator=(FrameBuffer&&) noexcept = default;
		FrameBuffer& operator=(const FrameBuffer&) = delete;

		glm::vec2 mSize{ 1.0f, 1.0f };
		std::shared_ptr<FrameBufferPlatformImpl> mImpl{};
	};
}
