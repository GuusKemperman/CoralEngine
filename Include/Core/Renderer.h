#pragma once
#include "Core/EngineSubsystem.h"

namespace CE
{
    struct RenderCommandQueue;
    struct StaticMeshPlatformImpl;
    struct TexturePlatformImpl;
    struct FrameBufferPlatformImpl;

    class Material;
    class StaticMesh;
    class Asset;

    template<typename T>
    class AssetHandle;

    class FrameBuffer;

	class Renderer final :
		public EngineSubsystem<Renderer>
	{
		friend EngineSubsystem;

		Renderer();

	public:
        // Thread-safe
        void AddStaticMesh(RenderCommandQueue& commandQueue,
            const AssetHandle<StaticMesh>& mesh,
            const AssetHandle<Material>& material,
            const glm::mat4& transform,
            glm::vec4 multiplicativeColor,
            glm::vec4 additiveColor);

        // Thread-safe
        void AddDirectionalLight(RenderCommandQueue& commandQueue, glm::vec3 direction, glm::vec4 color) const;

        // Thread-safe
        void AddPointLight(RenderCommandQueue& commandQueue, glm::vec3 position, float radius, glm::vec4 color) const;

        // Thread-safe
        void AddLine(RenderCommandQueue& commandQueue, glm::vec3 start, glm::vec3 end, glm::vec4 startColor, glm::vec4 endColor) const;

        // Thread-safe
        void SetRenderTarget(RenderCommandQueue& commandQueue, const glm::mat4& view, const glm::mat4& projection, FrameBuffer* renderTarget = nullptr);

        /**
		 * This is the only function in this file that is not thread-safe,
		 * as not all implementations supports submitting commands to the
		 * actual backend. Must be called from the main thread.
		 */
        void RunCommandQueues();

        // Thread-safe
        std::shared_ptr<RenderCommandQueue> CreateCommandQueue();

        // Must provide all attributes and the same number of vertices for each attribute.
        // Thread-safe
        std::shared_ptr<StaticMeshPlatformImpl> CreateStaticMeshPlatformImpl(std::span<const uint32> indices,
            std::span<const glm::vec3> positions,
            std::span<const glm::vec3> normals,
            std::span<const glm::vec3> tangents,
            std::span<const glm::vec2> textureCoordinates);

        // Thread-safe
        std::shared_ptr<TexturePlatformImpl> CreateTexturePlatformImpl(std::span<const std::byte> pixelsRGBA, glm::ivec2 size);

		// Thread-safe
        std::shared_ptr<TexturePlatformImpl> CreateTexturePlatformImpl(FrameBuffer&& frameBuffer);

		// Thread-safe
        std::shared_ptr<FrameBufferPlatformImpl> CreateFrameBufferPlatformImpl();

        // Can, for example, be used as an ImTextureID for rendering images.
		// Thread-safe
        void* GetPlatformId(TexturePlatformImpl* platformImpl);

        // Can, for example, be used as an ImTextureID for rendering images.
		// Thread-safe
        void* GetPlatformId(FrameBufferPlatformImpl* platformImpl);

    private:
        struct Impl;
        struct ImplDeleter
        {
            void operator()(Impl* impl) const;
        };
        std::unique_ptr<Impl, ImplDeleter> mImpl{};
	};
}
