#pragma once
#include "Meta/Fwd/MetaTypeIdFwd.h"

namespace CE
{
	class EngineConfig
	{
	public:
		EngineConfig() = default;
		EngineConfig(int argc, char** argv);

		std::vector<std::string> mProgramArguments{};

		std::string mGameDir{};

		bool mShouldRunUnitTests{};

		glm::ivec2 mWindowSizeHint = { 1920, 1080 };

		bool mIsFullScreen{};

		std::string mWindowTitle{};
	};

	class Engine
	{
	public:
		Engine(const EngineConfig& config);
		~Engine();

		void Run(Name starterLevel);
	};
}
