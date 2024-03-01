#include "Precomp.h"
#include "Core/FileIO.h"

using namespace Engine;
using namespace std;

FileIO::FileIO(int argc, char* argv[], const std::optional<std::string_view>& additionalAssetsDirectory)
{
	mPaths[Directory::EngineAssets] = std::string{ additionalAssetsDirectory.value_or("") };
	mPaths[Directory::GameAssets] = "Assets/";
	mPaths[Directory::Intermediate] = "Intermediate/";
	mPaths[Directory::ThisExecutable] = argc == 0 ? std::string{} : std::string{ argv[0] };
}

FileIO::~FileIO() = default;
