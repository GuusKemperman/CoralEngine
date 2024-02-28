#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

int main(int argc, char* args[])
{
	if (argc != 4)
	{
		std::cerr << "Invalid num of arguments provided" << std::endl;
		return 1;
	}

	const std::string_view macroName = args[1];
	const std::filesystem::path includeFolder = args[2];
	const std::filesystem::path destination = args[3];

	std::cout << "Collecting all " << macroName << " from " << includeFolder.string() << " and outputting to " << destination.string() << std::endl;

	if (!std::filesystem::exists(includeFolder))
	{
		std::cerr << "Include folder " << includeFolder.string() << "does not exist" << std::endl;
		return 1;
	}

	std::vector<std::filesystem::path> allFilesThatReflectOnStartup{};
	std::string line{};

	for (const std::filesystem::directory_entry& dirEntry : std::filesystem::recursive_directory_iterator(includeFolder))
	{
		if (!dirEntry.is_regular_file())
		{
			continue;
		}

		const std::filesystem::path& path = dirEntry.path();

		if (path.extension() != ".h"
			&& path.extension() != ".hpp")
		{
			continue;
		}

		std::ifstream file{path};

		if (!file.is_open())
		{
			std::cerr << "Could not open file " << path.string() << std::endl;
			continue;
		}

		while (!file.eof())
		{
			std::getline(file, line);
			if (line.find(macroName) != std::string::npos)
			{
				allFilesThatReflectOnStartup.emplace_back(path);
				break;
			}
		}
	}

	std::filesystem::create_directories(destination.parent_path());

	std::string newFileContents{};

	{
		std::stringstream newStr{};

		newStr << "#pragma once\n\n";

		for (const std::filesystem::path& path : allFilesThatReflectOnStartup)
		{
			newStr << "#include \"" << path.string() << "\"\n";
		}

		newFileContents = newStr.str();
	}

	std::string oldFileContents{};

	if (std::filesystem::exists(destination))
	{
		std::ifstream inFile{ destination };

		if (inFile.is_open())
		{
			std::stringstream oldStr{};
			oldStr << inFile.rdbuf();;
			oldFileContents = oldStr.str();
		}
	}

	// The file has no changes.
	// We do not want to write to it, to prevent an unnecessary recompile
	if (oldFileContents == newFileContents)
	{
		return 0;
	}

	std::cerr << "Changes detected, updating " << destination.string() << std::endl;

	std::ofstream outFile{ destination };

	if (!outFile.is_open())
	{
		std::cerr << "Could not write to destination file " << destination.string() << std::endl;
		return 1;
	}

	outFile << newFileContents;

	return 0;
}
