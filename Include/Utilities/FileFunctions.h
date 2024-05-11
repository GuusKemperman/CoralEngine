#pragma once
#include <filesystem>
#include <fstream>
#include <sstream>

namespace CE
{
	class FileFunctions
	{
	public:
		// Will create a new file if the given filepath does not exist yet, otherwise clears it
		// Returns true if failed
		static bool MakeEmpty(const std::string& filePath);

		// Returns true if failed.
		static bool Delete(const std::string& filePath);
		static bool DoesFileExist(const std::string& filePath);

		static bool IsFileNewer(const std::filesystem::path& file, const std::filesystem::path& reference);

		static std::string ReadFile(std::ifstream& fileStream)
		{
			std::stringstream buffers;
			buffers << fileStream.rdbuf();
			return buffers.str();
		}

		static std::string ReadFile(const char* filePath)
		{
			std::ifstream fileStream(filePath, std::ios::in);

			if (!fileStream.is_open())
			{
// Because we may want to use this outside of the engine as well
#ifdef LOG
				LOG(LogFileIO, Warning, "Could not read file {}; File does not exist.", filePath);
#endif // LOG
				return "";
			}

			std::string content = ReadFile(fileStream);
			fileStream.close();
			return content;
		}

		static std::string ReadFile(const std::filesystem::path& path)
		{
			return ReadFile(path.string().c_str());
		}

		static bool IsFileContentEqualTo(const std::filesystem::path& path, const std::string& contentToCompareAgainst)
		{
			const std::string contentsFile = ReadFile(path.string().c_str());
			return contentsFile == contentToCompareAgainst;
		}
	};
}
