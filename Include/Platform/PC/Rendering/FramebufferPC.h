#pragma once

class DXResource;
class DXHeapHandle;

namespace CE
{
	class FrameBuffer
	{
	public:
		FrameBuffer(glm::ivec2 initialSize = {1, 1});
		~FrameBuffer();

		void Bind() const;
		void Unbind() const;

		void BindSRVDepthToGraphics(int rootSlot) const;

		void Resize(glm::ivec2 newSize);
		glm::ivec2 GetSize() const { return mSize; }

		void Clear();

		void SetClearColor(glm::vec4 color) { mClearColor = color; }
		const glm::vec4 GetClearColor() { return mClearColor; }

		size_t GetColorTextureId();

	private:
		// Texture's can be created from
		// FrameBuffers. This involves
		// 'stealing' the DXHeapHandle.
		// We prefer using a friend
		// declaration over exposing the
		// stealing API to the user.
		friend class Texture;
		DXHeapHandle& GetCurrentHeapSlot();
		std::unique_ptr<DXResource>& GetCurrentResource();

		// Prevents having to include the very
		// large DX12 headers
		struct DXImpl;

		struct DXImplDeleter
		{
			void operator()(DXImpl* impl) const;
		};

		std::unique_ptr<DXImpl, DXImplDeleter> mImpl{};

		glm::vec4 mClearColor{ .39f, .45f, .5f, 1.0f };
		glm::ivec2 mSize{};
	};
}
