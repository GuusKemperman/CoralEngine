#include "Precomp.h"
#include "Utilities/FileFunctions.h"

bool CE::FileFunctions::MakeEmpty(const std::string& filePath)
{
	std::ofstream outFile(filePath);

	if (!outFile.is_open())
	{
		LOG(LogFileIO, Warning, "Could not create/open file {}. Make sure that the folder you're trying to create a file in exists. If the file already exists, it may be read only.", filePath.c_str());
		return true;
	}

	outFile.clear();
	outFile.close();

	return false;
}

bool CE::FileFunctions::Delete(const std::string& filePath)
{
	if (std::remove(filePath.c_str()))
	{
		LOG(LogFileIO, Warning, "Failed to delete file {}", filePath.c_str());
		return true;
	}

	return false;
}

bool CE::FileFunctions::DoesFileExist(const std::string& filePath)
{
	std::ifstream infile(filePath);
	return infile.good();
}

bool CE::FileFunctions::IsFileNewer(const std::filesystem::path& file, const std::filesystem::path& reference)
{
	if (!exists(reference)
		|| !exists(file))
	{
		return false;
	}

	return last_write_time(file) < last_write_time(reference);
}
