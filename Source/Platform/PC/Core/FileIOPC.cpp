#include "Precomp.h"
#include "Core/FileIO.h"

using namespace Engine;
using namespace std;

FileIO::FileIO(int argc, char* argv[], const std::string_view gameDir)
{
	mPaths[Directory::EngineAssets] = "Assets/";
	mPaths[Directory::GameAssets] = std::string{ gameDir } + "Assets/";
	mPaths[Directory::Intermediate] = std::string{ gameDir } + "Intermediate/";
	mPaths[Directory::ThisExecutable] = argc == 0 ? std::string{} : std::string{ argv[0] };
}

FileIO::~FileIO() = default;
