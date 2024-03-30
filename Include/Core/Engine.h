#pragma once

namespace CE
{
	class EngineClass
	{
	public:
		EngineClass(int argc, char* argv[], std::string_view gameDir);
		~EngineClass();

		void Run(Name starterLevel);
	};
}
