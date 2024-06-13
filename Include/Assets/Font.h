#pragma once
#include "Assets/Asset.h"

namespace CE
{
	class Texture;

	class Font final :
		public Asset
	{
	public:
		Font(std::string_view name);
		Font(AssetLoadInfo& loadInfo);

		~Font() = default;

		Font(Font&&) noexcept = delete;
		Font(const Font&) = delete;

		Font& operator=(Font&&) = delete;
		Font& operator=(const Font&) = delete;

		// The entirety of the original file, this buffer could 
		// for example be an entire .ttf file.
		const std::string& GetData() const { return mFileContents; }
		const stbtt_fontinfo& GetInfo() const { return mInfo; }
		const std::vector<stbtt_packedchar>& GetPackedCharacters() const { return mPackedCharacters; }
		const Texture& GetTexture() const { return *mTexture.get(); }
		float GetScale() const { return mScale; }

		static constexpr float sFontSize = 200.0f;
		static constexpr uint32_t sFontAtlasMinCharacters = 32;
		static constexpr uint32_t sFontAtlasNumCharacters = 96;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(Font);

		std::string mFileContents{};
		stbtt_fontinfo mInfo{};
		std::vector<stbtt_packedchar> mPackedCharacters{};
		std::unique_ptr<Texture> mTexture{};
		float mScale = 1.0f;
	};
}
