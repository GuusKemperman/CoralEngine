#include "Precomp.h"
#include "Core/Engine.h"

// For the UNIT_TEST_DECLARATION macro used in the Generated file
#include "Core/UnitTests.h"

// Forces initialization of otherwise unused static variables
#include "../Intermediate/Generated/Generated.h"

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