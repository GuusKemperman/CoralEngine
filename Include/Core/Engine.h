#pragma once

namespace Engine
{
	class EngineClass
	{
	public:
		EngineClass(int argc, char* argv[], 
			const std::optional<std::string_view>& additionalAssetsDirectory = std::nullopt);
		~EngineClass();

		void Run();
	};
}
