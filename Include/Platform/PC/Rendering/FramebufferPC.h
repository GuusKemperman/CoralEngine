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

		size_t GetColorTextureId();

	private:
		std::unique_ptr<DXResource> resource[FRAME_BUFFER_COUNT];
		std::unique_ptr<DXResource> depthResource;
		unsigned int frameBufferIndex[FRAME_BUFFER_COUNT];
		unsigned int frameBufferRscIndex[FRAME_BUFFER_COUNT];
		unsigned int depthStencilIndex;
		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandles[FRAME_BUFFER_COUNT];

		glm::vec4 mClearColor{};
		glm::ivec2 mSize{};
		D3D12_VIEWPORT mViewport;
		D3D12_RECT mScissorRect;

	};
}
#endif // EDITOR