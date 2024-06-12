#pragma once
#include "Assets/Asset.h"
#include <stb_image/stb_truetype.h>

namespace CE
{
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

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(Font);

		static constexpr float sFontSize = 50.0f;
		static constexpr uint32_t sFontAtlasMinCharacters = 32;
		static constexpr uint32_t sFontAtlasNumCharacters = 96;

		std::string mFileContents{};
		std::vector<stbtt_packedchar> mPackedCharacters{};
		stbtt_fontinfo mInfo{};
		float mScale = 1.0f;
	};
}
