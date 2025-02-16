#include "Precomp.h"
#include "Core/Renderer.h"

#include <glad/glad.h>

#include "Assets/Material.h"
#include "Assets/Texture.h"
#include "Assets/StaticMesh.h"
#include "Core/Device.h"
#include "Rendering/FrameBuffer.h"

namespace CE::Internal
{
	static GLuint CompileProgram(const GLchar* vertexSource, const GLchar* fragmentSource);
	static GLuint CompileShader(GLenum type, const GLchar* source);
	static void LinkProgram(GLuint program);
	static GLuint CompileStandardProgram();
	static GLuint CompileLinesProgram();
	static void CheckGLImpl(std::string_view file, uint32 line);
}

#ifdef ASSERTS_ENABLED
#define CHECK_GL CE::Internal::CheckGLImpl(__FILE__, __LINE__)
#else
#define CHECK_GL static_assert(true)
#endif

struct CE::TexturePlatformImpl
{
	~TexturePlatformImpl();

	struct InitData
	{
		std::vector<std::byte> mPixelsRGBA{};
	};
	std::unique_ptr<InitData> mPendingInit{};
	glm::ivec2 mSize{};
	GLuint mId{};
};

CE::TexturePlatformImpl::~TexturePlatformImpl()
{
	if (mPendingInit != nullptr)
	{
		return;
	}

	glDeleteTextures(1, &mId);
	CHECK_GL;
}

struct CE::StaticMeshPlatformImpl
{
	~StaticMeshPlatformImpl();

	struct InitData
	{
		std::vector<uint32> mIndices{};
		std::vector<glm::vec3> mPositions{};
		std::vector<glm::vec3> mNormals{};
		std::vector<glm::vec3> mTangents{};
		std::vector<glm::vec2> mTextureCoordinates{};
	};
	std::unique_ptr<InitData> mPendingInit{};
	GLuint mVao{};
	GLuint mEbo{};
	std::array<GLuint, 4> mVbos{};
	uint32_t mNumOfIndices{};
};

struct CE::FrameBufferPlatformImpl
{
	~FrameBufferPlatformImpl();

	void Resize(glm::ivec2 size);

	std::shared_ptr<TexturePlatformImpl> mColorTexture{};
	GLuint mFrameBuffer{};
	GLuint mDepthTexture{};
	bool mIsInitialized{};
};

CE::StaticMeshPlatformImpl::~StaticMeshPlatformImpl()
{
	if (mPendingInit != nullptr)
	{
		return;
	}

	glDeleteVertexArrays(1, &mVao);
	glDeleteBuffers(1, &mEbo);
	glDeleteBuffers(static_cast<GLsizei>(mVbos.size()), mVbos.data());
	CHECK_GL;
}

CE::FrameBufferPlatformImpl::~FrameBufferPlatformImpl()
{
	if (!mIsInitialized)
	{
		return;
	}

	glDeleteFramebuffers(1, &mFrameBuffer);
	glDeleteRenderbuffers(1, &mDepthTexture);
	CHECK_GL;
}

void CE::FrameBufferPlatformImpl::Resize(glm::ivec2 size)
{
	if (mColorTexture->mSize == size)
	{
		return;
	}

	mColorTexture->mSize = size;

	glBindTexture(GL_TEXTURE_2D, mColorTexture->mId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mColorTexture->mId, 0);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size.x, size.y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mColorTexture->mId, 0);

	glBindRenderbuffer(GL_RENDERBUFFER, mDepthTexture);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, size.x, size.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthTexture);
	CHECK_GL;
}

struct CE::RenderCommandQueue
{
	static constexpr int sMaxNumLines = 1 << 7;
	static constexpr int sNumFloatsPerLine = (3 + 4) * 2; // Pos + Colour * 2
	std::array<float, sMaxNumLines * sNumFloatsPerLine> mLines;
	std::atomic_int mTotalNumOfLinesRequested{};

	struct DirectionalLightEntry
	{
		glm::vec3 mDirection{};
		glm::vec4 mColor{};
	};
	std::vector<DirectionalLightEntry> mDirectionalLights{};
	std::mutex mDirectionalLightsMutex{};

	struct PointLightEntry
	{
		glm::vec3 mPosition{};
		float mRadius{};
		glm::vec4 mColor{};
	};
	std::vector<PointLightEntry> mPointLights{};
	std::mutex mPointLightsMutex{};

	struct StaticMeshEntry
	{
		glm::mat4 mTransform{};
		glm::vec4 mMultiplicativeColor{};
		glm::vec4 mAdditiveColor{};
		std::shared_ptr<StaticMeshPlatformImpl> mStaticMesh{};
		std::shared_ptr<TexturePlatformImpl> mBaseColorTexture{};
	};
	std::vector<StaticMeshEntry> mStaticMeshes{};
	std::mutex mStaticMeshesMutex{};

	struct RenderEntry
	{
		std::shared_ptr<FrameBufferPlatformImpl> mTarget{};
		// mRenderSize is unused if mTarget == nullptr
		glm::ivec2 mRenderSize{ -1, -1 };
		glm::mat4 mView{};
		glm::mat4 mProj{};
	};
	std::optional<RenderEntry> mRenderTargetEntry{};
	std::mutex mRenderTargetMutex{};

	glm::vec4 mClearColor{ .39f, .45f, .5f, 1.0f };
	std::mutex mClearColorMutex{};
};

struct CE::Renderer::Impl
{
	std::mutex mStaticMeshesMutex{};
	std::vector<std::shared_ptr<StaticMeshPlatformImpl>> mStaticMeshes{};

	std::mutex mTexturesMutex{};
	std::vector<std::shared_ptr<TexturePlatformImpl>> mTextures{};

	std::mutex mFrameBuffersMutex{};
	std::vector<std::shared_ptr<FrameBufferPlatformImpl>> mFrameBuffers{};

	std::mutex mCommandQueueMutex{};
	std::vector<std::shared_ptr<RenderCommandQueue>> mCommandQueues{};

	GLuint mStandardProgram{};
	GLuint mLinesProgram{};
	GLuint mLinesVAO{};
	GLuint mLinesVBO{};
};

CE::Renderer::Renderer() :
	mImpl(new Impl)
{
	if (Device::IsHeadless())
	{
		return;
	}

	// Consume any initial values
	CHECK_GL;

	mImpl->mStandardProgram = Internal::CompileStandardProgram();
	mImpl->mLinesProgram = Internal::CompileLinesProgram();

	glCreateVertexArrays(1, &mImpl->mLinesVAO);
	glBindVertexArray(mImpl->mLinesVAO);

	// Allocate VBO
	glGenBuffers(1, &mImpl->mLinesVBO);

	// Array buffer contains the attribute data
	glBindBuffer(GL_ARRAY_BUFFER, mImpl->mLinesVBO);

	static constexpr size_t sizePos = sizeof(glm::vec3);
	static constexpr size_t sizeCol = sizeof(glm::vec4);
	static constexpr size_t sizeTotal = sizePos + sizeCol;

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeTotal, nullptr);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeTotal, reinterpret_cast<void*>(sizePos));

	glBindVertexArray(0);

	glEnable(GL_DEPTH_TEST);
	glFrontFace(GL_CW);
	glEnable(GL_CULL_FACE);
	CHECK_GL;

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	CHECK_GL;
}

void CE::Renderer::AddStaticMesh(RenderCommandQueue& context, const AssetHandle<StaticMesh>& mesh,
	const AssetHandle<Material>& requestedMaterial, const glm::mat4& transform, glm::vec4 multiplicativeColor,
	glm::vec4 additiveColor)
{
	if (mesh == nullptr)
	{
		return;
	}

	const AssetHandle<Material> mat = requestedMaterial == nullptr ? Material::TryGetDefaultMaterial() : requestedMaterial;
	const AssetHandle<Texture> baseColor = mat == nullptr || mat->mBaseColorTexture == nullptr ? Texture::TryGetDefaultTexture() : mat->mBaseColorTexture;

	if (baseColor == nullptr)
	{
		return;
	}

	std::scoped_lock lock{ context.mStaticMeshesMutex };
	context.mStaticMeshes.emplace_back(transform, multiplicativeColor, additiveColor, mesh->GetPlatformImpl(), baseColor->GetPlatformImpl());
}

void CE::Renderer::AddDirectionalLight(RenderCommandQueue& context, glm::vec3 direction, glm::vec4 color) const
{
	std::scoped_lock lock{ context.mDirectionalLightsMutex };
	context.mDirectionalLights.emplace_back(direction, color);
}

void CE::Renderer::AddPointLight(RenderCommandQueue& context, glm::vec3 position, float radius, glm::vec4 color) const
{
	std::scoped_lock lock{ context.mPointLightsMutex };
	context.mPointLights.emplace_back(position, radius, color);
}

void CE::Renderer::AddLine(RenderCommandQueue& context, glm::vec3 start, glm::vec3 end, glm::vec4 startColor, glm::vec4 endColor) const
{
	const int numLines = context.mTotalNumOfLinesRequested++;
	if (numLines >= RenderCommandQueue::sMaxNumLines)
	{
		return;
	}

	float* data = &context.mLines[numLines * RenderCommandQueue::sNumFloatsPerLine];
	data[0] = start[0];
	data[1] = start[1];
	data[2] = start[2];
	data[3] = startColor[0];
	data[4] = startColor[1];
	data[5] = startColor[2];
	data[6] = startColor[3];
	data[7] = end[0];
	data[8] = end[1];
	data[9] = end[2];
	data[10] = endColor[0];
	data[11] = endColor[1];
	data[12] = endColor[2];
	data[13] = endColor[3];
}

void CE::Renderer::SetRenderTarget(RenderCommandQueue& context, const glm::mat4& view, const glm::mat4& projection, FrameBuffer* renderTarget)
{
	RenderCommandQueue::RenderEntry entry{};
	entry.mView = view;
	entry.mProj = projection;

	if (renderTarget != nullptr)
	{
		entry.mTarget = renderTarget->mImpl;
		entry.mRenderSize = renderTarget->mSize;
	}

	std::scoped_lock lock{ context.mRenderTargetMutex };
	context.mRenderTargetEntry.emplace(std::move(entry));
}

void CE::Renderer::SetClearColor(RenderCommandQueue& commandQueue, glm::vec4 color)
{
	std::scoped_lock lock{ commandQueue.mClearColorMutex };
	commandQueue.mClearColor = color;
}

void CE::Renderer::RunCommandQueues()
{
	const std::scoped_lock lock
	{
		mImpl->mTexturesMutex,
		mImpl->mStaticMeshesMutex,
		mImpl->mCommandQueueMutex,
		mImpl->mFrameBuffersMutex
	};

	static constexpr auto isLastReference = [](const auto& sharedPtr) -> bool
		{
			return sharedPtr.use_count() <= 1;
		};

	const auto cleanUp = [this]
		{
			std::erase_if(mImpl->mTextures, isLastReference);
			std::erase_if(mImpl->mStaticMeshes, isLastReference);
			std::erase_if(mImpl->mCommandQueues, isLastReference);
			std::erase_if(mImpl->mFrameBuffers, isLastReference);
		};

	if (Device::IsHeadless())
	{
		cleanUp();
		return;
	}

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	{
		std::erase_if(mImpl->mTextures, isLastReference);

		for (const std::shared_ptr<TexturePlatformImpl>& texture : mImpl->mTextures)
		{
			if (texture->mPendingInit == nullptr)
			{
				continue;
			}

			// Create an OpenGL texture
			glGenTextures(1, &texture->mId);
			glBindTexture(GL_TEXTURE_2D, texture->mId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture->mSize.x, texture->mSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->mPendingInit->mPixelsRGBA.data());
			glGenerateMipmap(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, 0);
			CHECK_GL;

			texture->mPendingInit.reset();
		}
	}

	{
		std::erase_if(mImpl->mStaticMeshes, isLastReference);

		for (const std::shared_ptr<StaticMeshPlatformImpl>& staticMesh : mImpl->mStaticMeshes)
		{
			if (staticMesh->mPendingInit == nullptr)
			{
				continue;
			}

			glGenVertexArrays(1, &staticMesh->mVao);
			glBindVertexArray(staticMesh->mVao);

			// Create the index buffer
			glGenBuffers(1, &staticMesh->mEbo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, staticMesh->mEbo);

			staticMesh->mNumOfIndices = static_cast<int>(staticMesh->mPendingInit->mIndices.size());
			glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				staticMesh->mPendingInit->mIndices.size() * sizeof(uint32),
				staticMesh->mPendingInit->mIndices.data(), GL_STATIC_DRAW);

			// Create the vertex buffers	
			glGenBuffers((GLsizei)staticMesh->mVbos.size(), staticMesh->mVbos.data());

			const int vertexCount = static_cast<int>(staticMesh->mPendingInit->mPositions.size());

			// Create the position buffer
			glBindBuffer(GL_ARRAY_BUFFER, staticMesh->mVbos[0]);
			glBufferData(GL_ARRAY_BUFFER, vertexCount * 3 * sizeof(float), staticMesh->mPendingInit->mPositions.data(), GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

			// Create the normal buffer
			if (static_cast<int>(staticMesh->mPendingInit->mNormals.size()) == vertexCount)
			{
				glBindBuffer(GL_ARRAY_BUFFER, staticMesh->mVbos[1]);
				glBufferData(GL_ARRAY_BUFFER, vertexCount * 3 * sizeof(float), staticMesh->mPendingInit->mNormals.data(), GL_STATIC_DRAW);
				glEnableVertexAttribArray(1);
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
			}

			// Create the texture coordinate buffer
			if (static_cast<int>(staticMesh->mPendingInit->mTextureCoordinates.size()) == vertexCount)
			{
				glBindBuffer(GL_ARRAY_BUFFER, staticMesh->mVbos[2]);
				glBufferData(GL_ARRAY_BUFFER, vertexCount * 2 * sizeof(float), staticMesh->mPendingInit->mTextureCoordinates.data(), GL_STATIC_DRAW);
				glEnableVertexAttribArray(2);
				glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
			}

			// Create the tangents buffer
			if (static_cast<int>(staticMesh->mPendingInit->mTangents.size()) == vertexCount)
			{
				glBindBuffer(GL_ARRAY_BUFFER, staticMesh->mVbos[3]);
				glBufferData(GL_ARRAY_BUFFER, vertexCount * 3 * sizeof(float), staticMesh->mPendingInit->mTangents.data(), GL_STATIC_DRAW);
				glEnableVertexAttribArray(3);
				glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
			}

			// Unbind the vertex array
			glBindVertexArray(0);
			staticMesh->mPendingInit.reset();
			CHECK_GL;
		}
	}

	{
		std::erase_if(mImpl->mFrameBuffers, isLastReference);

		for (const std::shared_ptr<FrameBufferPlatformImpl>& frameBuffer : mImpl->mFrameBuffers)
		{
			if (frameBuffer->mIsInitialized)
			{
				continue;
			}

			glGenFramebuffers(1, &frameBuffer->mFrameBuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer->mFrameBuffer);

			glGenTextures(1, &frameBuffer->mColorTexture->mId);
			glGenRenderbuffers(1, &frameBuffer->mDepthTexture);

			frameBuffer->Resize({ 1, 1 });

			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, frameBuffer->mColorTexture->mId, 0);

			const GLenum drawBuffer = GL_COLOR_ATTACHMENT0;
			glDrawBuffers(1, &drawBuffer);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			frameBuffer->mIsInitialized = true;
			CHECK_GL;
		}
	}

	for (const std::shared_ptr<RenderCommandQueue>& commandQueue : mImpl->mCommandQueues)
	{
		if (!commandQueue->mRenderTargetEntry.has_value())
		{
			continue;
		}

		std::scoped_lock commandQueueLock
		{
			commandQueue->mPointLightsMutex,
			commandQueue->mDirectionalLightsMutex,
			commandQueue->mRenderTargetMutex,
			commandQueue->mStaticMeshesMutex
		};

		FrameBufferPlatformImpl* target = commandQueue->mRenderTargetEntry->mTarget.get();

		if (target != nullptr)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, target->mFrameBuffer);
			target->Resize(commandQueue->mRenderTargetEntry->mRenderSize);
			glViewport(0, 0, target->mColorTexture->mSize.x, target->mColorTexture->mSize.y);
		}

		// Clear the screen
		const glm::vec4 clearColor = commandQueue->mClearColor;
		glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		
		CHECK_GL;

		// Activate the standard program
		const GLuint program = mImpl->mStandardProgram;
		glUseProgram(program);

		// Send the view and projection matrices to the standard program
		glUniformMatrix4fv(glGetUniformLocation(program, "u_view"), 1, GL_FALSE, &commandQueue->mRenderTargetEntry->mView[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(program, "u_projection"), 1, GL_FALSE, &commandQueue->mRenderTargetEntry->mProj[0][0]);
		const GLint texture_location = glGetUniformLocation(program, "u_texture");

		{
			// Send the directional lights to the standard program
			glUniform1i(glGetUniformLocation(program, "u_num_directional_lights"), static_cast<int>(commandQueue->mDirectionalLights.size()));

			for (int i = 0; i < static_cast<int>(commandQueue->mDirectionalLights.size()); ++i)
			{
				std::string name = "u_directional_lights[" + std::to_string(i) + "].direction";
				glUniform3fv(glGetUniformLocation(program, name.c_str()), 1, &commandQueue->mDirectionalLights[i].mDirection[0]);
				name = "u_directional_lights[" + std::to_string(i) + "].color";
				glUniform3fv(glGetUniformLocation(program, name.c_str()), 1, &commandQueue->mDirectionalLights[i].mColor[0]);
			}

			commandQueue->mDirectionalLights.clear();
		}

		{
			// Send the point lights to the standard program	
			glUniform1i(glGetUniformLocation(program, "u_num_point_lights"), static_cast<int>(commandQueue->mPointLights.size()));

			for (int i = 0; i < static_cast<int>(commandQueue->mPointLights.size()); ++i)
			{
				std::string name = "u_point_lights[" + std::to_string(i) + "]";
				glUniform3fv(glGetUniformLocation(program, (name + ".position").c_str()),
					1, &commandQueue->mPointLights[i].mPosition[0]);
				glUniform1f(glGetUniformLocation(program, (name + ".radius").c_str()),
					commandQueue->mPointLights[i].mRadius);
				glUniform3fv(glGetUniformLocation(program, (name + ".color").c_str()),
					1, &commandQueue->mPointLights[i].mColor[0]);
			}

			commandQueue->mPointLights.clear();
		}

		{
			// SetRenderTarget each entry
			for (const RenderCommandQueue::StaticMeshEntry& entry : commandQueue->mStaticMeshes)
			{
				if (entry.mStaticMesh == nullptr
					|| entry.mBaseColorTexture == nullptr)
				{
					continue;
				}

				// Bind the vertex array
				glBindVertexArray(entry.mStaticMesh->mVao);

				// Bind the texture
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, entry.mBaseColorTexture->mId);
				// texture_location
				glUniform1i(texture_location, 0);

				// Send the model matrix to the standard program
				glUniformMatrix4fv(glGetUniformLocation(program, "u_model"), 1, GL_FALSE, &entry.mTransform[0][0]);

				// Send the multiply and add colors to the standard program
				glUniform4fv(glGetUniformLocation(program, "u_mul_color"), 1, &entry.mMultiplicativeColor[0]);
				glUniform4fv(glGetUniformLocation(program, "u_add_color"), 1, &entry.mAdditiveColor[0]);

				// Draw the mesh
				glDrawElements(GL_TRIANGLES, entry.mStaticMesh->mNumOfIndices, GL_UNSIGNED_INT, 0);
			}

			commandQueue->mStaticMeshes.clear();
		}

		// SetRenderTarget debug lines
		const int numOfLines = std::min<int>(commandQueue->mTotalNumOfLinesRequested, RenderCommandQueue::sMaxNumLines);

		if (numOfLines > 0)
		{
			const glm::mat4 vp = commandQueue->mRenderTargetEntry->mProj * commandQueue->mRenderTargetEntry->mView;
			glUseProgram(mImpl->mLinesProgram);

			glUniformMatrix4fv(glGetUniformLocation(mImpl->mLinesProgram, "u_worldviewproj"), 1, GL_FALSE, &vp[0][0]);
			glBindVertexArray(mImpl->mLinesVAO);
			
			glBindBuffer(GL_ARRAY_BUFFER, mImpl->mLinesVBO);
			const int numOfBytes = numOfLines * RenderCommandQueue::sNumFloatsPerLine * sizeof(float);
			glBufferData(GL_ARRAY_BUFFER, numOfBytes, commandQueue->mLines.data(), GL_STREAM_DRAW);
			glDrawArrays(GL_LINES, 0, numOfLines * 2);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glUseProgram(0);

			commandQueue->mTotalNumOfLinesRequested = 0;
		}

		commandQueue->mRenderTargetEntry.reset();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		const glm::ivec2 screenSize = Device::Get().GetDisplaySize();
		glViewport(0, 0, screenSize.x, screenSize.y);
		CHECK_GL;
	}

	cleanUp();
}

std::shared_ptr<CE::RenderCommandQueue> CE::Renderer::CreateCommandQueue()
{
	std::scoped_lock lock{ mImpl->mCommandQueueMutex };
	return mImpl->mCommandQueues.emplace_back(std::make_shared<RenderCommandQueue>());
}

std::shared_ptr<CE::StaticMeshPlatformImpl> CE::Renderer::CreateStaticMeshPlatformImpl(std::span<const uint32> indices,
	std::span<const glm::vec3> positions, std::span<const glm::vec3> normals, std::span<const glm::vec3> tangents,
	std::span<const glm::vec2> textureCoordinates)
{
	std::unique_ptr<StaticMeshPlatformImpl::InitData> initData = std::make_unique<StaticMeshPlatformImpl::InitData>(
		std::vector<uint32>{ indices.begin(), indices.end() },
		std::vector<glm::vec3>{ positions.begin(), positions.end() },
		std::vector<glm::vec3>{ normals.begin(), normals.end() },
		std::vector<glm::vec3>{ tangents.begin(), tangents.end() },
		std::vector<glm::vec2>{ textureCoordinates.begin(), textureCoordinates.end() });

	std::scoped_lock lock{ mImpl->mStaticMeshesMutex };
	return mImpl->mStaticMeshes.emplace_back(std::make_shared<StaticMeshPlatformImpl>(std::move(initData)));
}

std::shared_ptr<CE::TexturePlatformImpl> CE::Renderer::CreateTexturePlatformImpl(std::span<const std::byte> pixelsRGBA,
                                                                                   glm::ivec2 size)
{
	if (size.x * size.y * 4 != static_cast<int>(pixelsRGBA.size()))
	{
		LOG(LogCore, Error, "Size {}x{} does not match number of pixels provided", size.x, size.y, pixelsRGBA.size());
		return nullptr;
	}

	std::unique_ptr<TexturePlatformImpl::InitData> initData = std::make_unique<TexturePlatformImpl::InitData>(
		std::vector<std::byte>{ pixelsRGBA.data(), pixelsRGBA.data() + pixelsRGBA.size() });
	
	std::scoped_lock lock{ mImpl->mTexturesMutex };
	return mImpl->mTextures.emplace_back(std::make_shared<TexturePlatformImpl>(std::move(initData), size));
}

std::shared_ptr<CE::TexturePlatformImpl> CE::Renderer::CreateTexturePlatformImpl(FrameBuffer&& frameBuffer)
{
	const std::shared_ptr<FrameBufferPlatformImpl> frameBufferImpl = std::move(frameBuffer.mImpl);

	if (frameBufferImpl == nullptr)
	{
		return nullptr;
	}
	return frameBufferImpl->mColorTexture;
}

std::shared_ptr<CE::FrameBufferPlatformImpl> CE::Renderer::CreateFrameBufferPlatformImpl()
{
	std::shared_ptr<TexturePlatformImpl> colorTexture = [this]
		{
			std::scoped_lock lock{ mImpl->mTexturesMutex };
			return mImpl->mTextures.emplace_back(std::make_shared<TexturePlatformImpl>());
		}();
	
	std::scoped_lock lock{ mImpl->mFrameBuffersMutex };
	return mImpl->mFrameBuffers.emplace_back(std::make_shared<FrameBufferPlatformImpl>(std::move(colorTexture)));
}

void* CE::Renderer::GetPlatformId(const AssetHandle<Texture>& texture)
{
	if (texture == nullptr
		|| texture->GetPlatformImpl() == nullptr)
	{
		return nullptr;
	}

	return reinterpret_cast<void*>(static_cast<intptr>(texture->GetPlatformImpl()->mId));
}

void* CE::Renderer::GetPlatformId(const FrameBuffer& frameBuffer)
{
	if (frameBuffer.mImpl == nullptr
		|| frameBuffer.mImpl->mColorTexture == nullptr)
	{
		return nullptr;
	}

	return  reinterpret_cast<void*>(static_cast<intptr>(frameBuffer.mImpl->mColorTexture->mId));
}

void CE::Renderer::ImplDeleter::operator()(Impl* impl) const
{
	delete impl;
}

GLuint CE::Internal::CompileProgram(const GLchar* vertexSource, const GLchar* fragmentSource)
{
	GLuint program = glCreateProgram();
	GLuint vert = CompileShader(GL_VERTEX_SHADER, vertexSource);
	GLuint frag = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

	glAttachShader(program, vert);
	glAttachShader(program, frag);

	LinkProgram(program);

	glDeleteShader(vert);
	glDeleteShader(frag);
	CHECK_GL;

	return program;
}

GLuint CE::Internal::CompileShader(GLenum type, const GLchar* source)
{
	const GLuint shader = glCreateShader(type);

	glShaderSource(shader, 1, &source, nullptr);
	glCompileShader(shader);

	GLint logLength = 0;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength > 1)
	{
		GLchar* log = static_cast<GLchar*>(malloc(logLength));
		glGetShaderInfoLog(shader, logLength, &logLength, log);
		LOG(LogCore, Fatal, "Program failed to compile - {}", log);
		free(log);
	}

	GLint status{};
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	ASSERT(status != 0);
	return shader;
}

void CE::Internal::LinkProgram(GLuint program)
{
	GLint status;
	glLinkProgram(program);

	GLint logLength = 0;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength > 1)
	{
		GLchar* log = static_cast<GLchar*>(malloc(logLength));
		glGetProgramInfoLog(program, logLength, &logLength, log);
		LOG(LogCore, Fatal, "Program failed to link - {}", log);
		free(log);
	}

	glGetProgramiv(program, GL_LINK_STATUS, &status);
	ASSERT(status != 0);
}

GLuint CE::Internal::CompileStandardProgram()
{
	return CompileProgram(
		R"(
#version 460 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texture_coordinate;
layout(location = 3) in vec3 a_tangents;

out vec3 v_normal;
out vec2 v_texture_coordinate;
out vec3 v_frag_position;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

void main()
{
    // Transform position to world space and then to clip space
    vec4 world_position = u_model * vec4(a_position, 1.0);
    gl_Position = u_projection * u_view * world_position;

    // Pass the transformed normal to the fragment shader
    v_normal = mat3(transpose(inverse(u_model))) * a_normal;

    // Pass the world position to the fragment shader for lighting calculations
    v_frag_position = vec3(world_position);

    // Pass the texture coordinates to the fragment shader
    v_texture_coordinate = a_texture_coordinate;

}
	)",
		R"(
#version 460 core

in vec3 v_normal;
in vec2 v_texture_coordinate;
in vec3 v_frag_position;
in vec3 v_color;

out vec4 frag_color;

uniform sampler2D u_texture;
uniform int u_num_directional_lights;
uniform int u_num_point_lights;
uniform vec4 u_mul_color;
uniform vec4 u_add_color;

const int MAX_DIR_LIGHTS = 4;
const int MAX_POINT_LIGHTS = 4;

struct directional_light {
    vec3 direction;
    vec3 color;
};

struct point_light {
    vec3 position;
    vec3 color;
    float radius;
};

uniform directional_light u_directional_lights[MAX_DIR_LIGHTS];
uniform point_light u_point_lights[MAX_POINT_LIGHTS];

void main()
{
    // Normalize the interpolated normal
    vec3 normal = normalize(v_normal);

    // Start with the ambient color
    vec4 ambient = vec4(0.1, 0.1, 0.1, 0.0);

    // Initialize diffuse and specular components
    vec4 diffuse = vec4(0.0);
    vec4 specular = vec4(0.0);

    // Calculate directional lights contribution
    for (int i = 0; i < u_num_directional_lights; ++i) 
	{
        vec3 light_dir = normalize(-u_directional_lights[i].direction);
        // Diffuse component
        float diff = max(dot(normal, light_dir), 0.0);
        diffuse += vec4(diff * u_directional_lights[i].color, 1.0);
    }

    // Combine all components
	vec4 base = u_add_color + u_mul_color * texture(u_texture, v_texture_coordinate).rgba;
    vec4 result = base * (ambient + diffuse + specular);
    frag_color = result;
}
	)");
}

GLuint CE::Internal::CompileLinesProgram()
{
	return CompileProgram(
		R"(#version 460 core
		layout (location = 0) in vec3 a_position;
		layout (location = 1) in vec4 a_color;
		
		uniform mat4 u_worldviewproj;

		out vec4 v_color;

		void main()
		{
			v_color = a_color;
			gl_Position = u_worldviewproj * vec4(a_position, 1.0);
		})",
		R"(#version 460 core
		in vec4 v_color;
		out vec4 frag_color;
		
		void main()
		{
			frag_color = v_color;
		})");
}

void CE::Internal::CheckGLImpl(std::string_view file, uint32 line)
{
	const GLenum error = glGetError();
	std::string_view errorStr = "UNKNOWN ERROR";

	if (error != GL_NO_ERROR)
	{
		switch (error)
		{
		case GL_INVALID_ENUM: errorStr = "INVALID ENUM"; break;
		case GL_INVALID_OPERATION: errorStr = "INVALID OPERATION"; break;
		case GL_INVALID_VALUE: errorStr = "INVALID VALUE"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: errorStr = "INVALID FRAMEBUFFER OPERATION"; break;
		case GL_INVALID_INDEX: errorStr = "INVALID INDEX"; break;
		default:;
		}

		Logger::Get().Log(Format("GLErrorCode {} - {}", error, errorStr), "LogCore", Error, file, line);
	}

	const int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	errorStr = {};

	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		switch (status)
		{
		case GL_FRAMEBUFFER_UNDEFINED: errorStr = ("GL_FRAMEBUFFER_UNDEFINED");	break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: errorStr = ("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");	break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: errorStr = ("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");	break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: errorStr = ("GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER");	break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: errorStr = ("GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER");	break;
		case GL_FRAMEBUFFER_UNSUPPORTED: errorStr = ("GL_FRAMEBUFFER_UNSUPPORTED");	break;
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: errorStr = ("GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE");	break;
		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: errorStr = ("GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS");	break;
		default:;
		}

		Logger::Get().Log(Format("Invalid framebuffer {} - {}", error, errorStr), "LogCore", Error, file, line);
	}
}
