#pragma once

namespace CE
{
	class Engine
	{
	public:
		Engine(int argc, char* argv[], std::string_view gameDir);
		~Engine();

		void Run(Name starterLevel);
	};
}
