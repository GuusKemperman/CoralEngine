#include "Precomp.h"
#include "Core/Engine.h"

int main(int argc, char* argv[])
{
	std::cout << "Entered main - GameDir: " << GAME_DIR << std::endl;

	try
	{
		CE::Engine engine{ argc, argv, GAME_DIR };
		engine.Run("L_MainMenu");
	}
	catch ([[maybe_unused]] const std::exception& e)
	{
		std::cerr << "Fatal exception - " << e.what() << std::endl;
		return 904;
	}
	
	return 0;
}