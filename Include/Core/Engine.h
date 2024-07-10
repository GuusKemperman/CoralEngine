#pragma once

namespace Engine
{
	struct GeneratedEntryPoints;

	class EngineClass
	{
	public:
		EngineClass(int argc, char* argv[]);
		~EngineClass();

		void Run();
	};
}
