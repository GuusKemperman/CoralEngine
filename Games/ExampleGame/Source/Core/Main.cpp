#include "Precomp.h"
#include "Core/Engine.h"

// For the UNIT_TEST_DECLARATION macro used in the Generated file
#include "Core/UnitTests.h"

// Forces initialization of otherwise unused static variables
#include "../Intermediate/Generated/Generated.h"

int main(int argc, char* argv[])
{
	if (argc >= 2
		&& strcmp(argv[1], "run_tests") == 0)
	{
		try
		{
			CE::Engine engine{ argc, argv, GAME_DIR };
			char* nullpt = nullptr;
			*nullpt = 0;
			engine.Run("L_MainMenu");
		}
		catch (const std::exception& e)
		{
			std::cout << e.what() << std::endl;
			return 1;
		}
		catch (...)
		{
			std::cout << "Unknown exception" << std::endl;
			return 1;
		}
	}
	else
	{
		CE::Engine engine{ argc, argv, GAME_DIR };
		engine.Run("L_MainMenu");
	}

	return 0;
}