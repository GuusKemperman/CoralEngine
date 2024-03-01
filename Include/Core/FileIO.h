#pragma once
#include "Core/EngineSubsystem.h"

#include <map>
#include <string>
#include <vector>

namespace Engine
{
	/// <summary>
	/// The FileIO class provides a cross-platform way to read and write files.
	/// </summary>
	class FileIO :
		public EngineSubsystem<FileIO>
	{
		friend class EngineSubsystem;

		/// <summary>
		/// Initializes the File IO system, filling in all the paths and
		/// mounting the virtual file system on platforms that need it.
		/// </summary>
		FileIO(int argc, char* argv[], const std::optional<std::string_view>& additionalAssetsDirectory = std::nullopt);

		/// <summary>
		/// Unmount the virtual file system and shut down the File IO system.
		/// </summary>
		~FileIO();

	public:
		/// <summary>
		/// Types of paths that can be used. These can be very different on different platforms.
		/// </summary>
		enum class Directory
		{
			EngineAssets,
			GameAssets,
			Intermediate,
			ThisExecutable,
		};

		/// <summary>
		/// Read a text file into a string. The string is empty if the file was not found.
		/// </summary>
		std::string ReadTextFile(Directory type, const std::string& path);

		/// <summary>
		/// Write a string to a text file. The file is created if it does not exist.
		/// Returns true if the file was written successfully.
		/// </summary>
		bool WriteTextFile(Directory type, const std::string& path, const std::string& content);

		/// <summary>
		/// Read a binary file into a string. The vector is empty if the file was not found.
		/// </summary>
		std::vector<char> ReadBinaryFile(Directory type, const std::string& path);

		/// <summary>
		/// Write a string to a binary file. The file is created if it does not exist.
		/// Returns true if the file was written successfully.
		/// </summary>
		bool WriteBinaryFile(Directory type, const std::string& path, const std::vector<char>& content);

		/// <summary>
		/// Get the full path of a file.
		/// </summary>
		std::string GetPath(Directory type, const std::string& path);

		/// <summary>
		/// Check if a file exists.
		/// </summary>
		bool Exists(Directory type, const std::string& path);

		/// <summary>
		/// Check the last time a file was modified. Only used on desktop platforms.
		/// </summary>
		uint64_t LastModified(Directory type, const std::string& path);

	private:

		// Roots of the paths per type, stored in a map.
		std::map<Directory, std::string> mPaths;
	};

}  // namespace Engine
