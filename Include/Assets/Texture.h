#pragma once
#include "Assets/Asset.h"
#include "xsr.hpp"

namespace Engine
{
	class Texture final :
		public Asset
	{
	public:
		Texture(std::string_view name);
		Texture(AssetLoadInfo& loadInfo);
		~Texture() override;

		Texture(Texture&& other) noexcept;
		Texture(const Texture&) = delete;

		Texture& operator=(Texture&&) = delete;
		Texture& operator=(const Texture&) = delete;

		const xsr::texture_handle& GetHandle() const { return mTextureHandle; }
		void SetHandle(xsr::texture_handle&& handle) { mTextureHandle = handle; }

	private:
		xsr::texture_handle mTextureHandle{};

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(Texture);
	};
}  // namespace Engine
