#include "Precomp.h"

#include "Core/Engine.h"
#include "Core/FileIO.h"

using namespace CE;
using namespace std;

FileIO::FileIO(const EngineConfig& config)
{
	mPaths[Directory::EngineAssets] = "Assets/";
	mPaths[Directory::GameAssets] = std::string{ config.mGameDir } + "Assets/";
	mPaths[Directory::Intermediate] = std::string{ config.mGameDir } + "Intermediate/";
	mPaths[Directory::ThisExecutable] = std::string{ !config.mProgramArguments.empty() ? config.mProgramArguments[0] : std::string_view{} };
}

FileIO::~FileIO() = default;
