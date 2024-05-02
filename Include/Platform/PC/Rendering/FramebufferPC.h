#ifdef EDITOR
#pragma once
#include "DX12Classes/DXDefines.h"
#include "glm/glm.hpp"
#include "DX12Classes/DXResource.h"
#include "DX12Classes/DXHeapHandle.h"

namespace CE
{
	class MyShader;

	class FrameBuffer
	{
	public:
		FrameBuffer(glm::ivec2 initialSize = {1, 1});
		~FrameBuffer();

		void Bind() const;
		void Unbind() const;

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

		std::unique_ptr<DXResource> mResource[FRAME_BUFFER_COUNT];
		std::unique_ptr<DXResource> mDepthResource;
		DXHeapHandle mFrameBufferHandle[FRAME_BUFFER_COUNT];
		DXHeapHandle mFrameBufferRscHandle[FRAME_BUFFER_COUNT];
		DXHeapHandle mDepthStencilHandle;
		DXHeapHandle mDepthStencilSRVHandle;

		glm::vec4 mClearColor{ .39f, .45f, .5f, 1.0f };
		glm::ivec2 mSize{};
		D3D12_VIEWPORT mViewport;
		D3D12_RECT mScissorRect;

	};
}
#endif // EDITOR