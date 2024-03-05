#pragma once

namespace Engine
{
	class EngineClass
	{
	public:
		EngineClass(int argc, char* argv[], std::string_view gameDir);
		~EngineClass();

		void Run();
	};
}
