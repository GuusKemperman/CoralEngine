#pragma once
#include "Assets/Asset.h"

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

		std::string mFileContents{};
	};
}
