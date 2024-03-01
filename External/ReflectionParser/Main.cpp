#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>

std::string_view sUnitTestMacroName{};
std::string_view sReflectMacroName{};

void ParseDir(const std::filesystem::path& directory, std::ostream& output, std::function<bool(const std::filesystem::path&, std::string_view, std::ostream&)> lineParser);

bool SourceLineParser(const std::filesystem::path& path, std::string_view line, std::ostream& output);

bool HeaderLineParser(const std::filesystem::path& path, std::string_view line, std::ostream& output);

int main(int argc, char* args[])
{
	if (argc != 6)
	{
		std::cerr << "Invalid num of arguments provided" << std::endl;
		return 1;
	}

	sReflectMacroName = args[1];
	sUnitTestMacroName = args[2];
	const std::filesystem::path includeFolder = args[3];
	const std::filesystem::path sourceFolder = args[4];
	const std::filesystem::path destination = args[5];

	//std::cout << "Collecting all " << reflectOnStartUpName << " from " << includeFolder.string() << " and outputting to " << destination.string() << std::endl;

	std::string newFileContents{};

	{
		std::stringstream newStr{};

		newStr << "#pragma once\n\n";

		ParseDir(includeFolder, newStr, HeaderLineParser);
		ParseDir(sourceFolder, newStr, SourceLineParser);

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

	std::cout << "Changes detected, updating " << destination.string() << std::endl;

	std::filesystem::create_directories(destination.parent_path());
	std::ofstream outFile{ destination };

	if (!outFile.is_open())
	{
		std::cerr << "Could not write to destination file " << destination.string() << std::endl;
		return 1;
	}

	outFile << newFileContents;

	return 0;
}


void ParseDir(const std::filesystem::path& directory, std::ostream& output, std::function<bool(const std::filesystem::path&, std::string_view, std::ostream&)> lineParser)
{
	if (!std::filesystem::exists(directory))
	{
		std::cerr << "Folder " << directory.string() << "does not exist" << std::endl;
		exit(1);
	}

	std::string line{};

	for (const std::filesystem::directory_entry& dirEntry : std::filesystem::recursive_directory_iterator(directory))
	{
		if (!dirEntry.is_regular_file())
		{
			continue;
		}

		const std::filesystem::path& path = dirEntry.path();

		std::ifstream file{ path };

		if (!file.is_open())
		{
			std::cerr << "Could not open file " << path.string() << std::endl;
			continue;
		}

		while (!file.eof())
		{
			std::getline(file, line);

			if (lineParser(path, line, output))
			{
				break;
			}
		}
	}
}

bool SourceLineParser(const std::filesystem::path& path, std::string_view line, std::ostream& output)
{
	const size_t macroPos = line.find(sUnitTestMacroName);

	if (macroPos == std::string::npos)
	{
		return false;
	}

	const size_t argsStart = line.find('(', macroPos);

	if (argsStart == std::string::npos)
	{
		return false;
	}

	const size_t argsEnd = line.find(')', argsStart);

	if (argsEnd == std::string::npos)
	{
		return false;
	}

	const std::string_view testName = line.substr(argsStart + 1, argsEnd - argsStart - 1);

	output << "UNIT_TEST_DECLARATION(" << testName << ");\n";
	return false;
}

bool HeaderLineParser(const std::filesystem::path& path, std::string_view line, std::ostream& output)
{
	if (line.find(sReflectMacroName) != std::string::npos)
	{
		output << "#include \"" << path.string() << "\"\n";
		return true;
	}
	return false;
}