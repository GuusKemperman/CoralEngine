#include <filesystem>

#include "Core/FileIO.h"

using namespace Engine;
using namespace std;

FileIO::FileIO(std::string_view pathToThisExecutable)
{ 
	mPaths[Directory::Asset] = "assets/";
	mPaths[Directory::Intermediate] = "intermediate/";
	mPaths[Directory::ThisExecutable] = std::string{ pathToThisExecutable };
}

FileIO::~FileIO() = default;
