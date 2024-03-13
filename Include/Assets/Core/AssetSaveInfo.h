#pragma once
#include <sstream>

namespace Engine
{
	class MetaType;

	class AssetSaveInfo
	{
	public:
		AssetSaveInfo(const std::string& name, const MetaType& assetClass);
		AssetSaveInfo(const std::string& name, const MetaType& assetClass, 
			const std::filesystem::path& importedFromFile, uint32 importerVersion);
		
		AssetSaveInfo(AssetSaveInfo&& other) noexcept;
		AssetSaveInfo(const AssetSaveInfo&) = delete;

		AssetSaveInfo& operator=(AssetSaveInfo&& other) noexcept;
		AssetSaveInfo& operator=(const AssetSaveInfo&) = delete;

		// Returns true on success
		bool SaveToFile(const std::filesystem::path& path) const;
		
		bool IsEmpty() const;

		std::ostream& GetStream() { return mStream; }
		const std::ostream& GetStream() const { return mStream; }

		std::string ToString() const;
		
	private:
		// First save to a string stream; if anything goes wrong, our original file is left unmodified
		std::ostringstream mStream{};
	};
}