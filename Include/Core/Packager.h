#pragma once

namespace Engine
{
	class Packager
	{
	public:
		// Returns true on success
		static bool Package(const std::filesystem::path& compiledExecutableToPackage, 
			const std::filesystem::path& outputDirectory, 
			std::vector<std::filesystem::path> assetsToPackage);
	};
}